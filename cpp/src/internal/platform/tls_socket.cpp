#include "internal/platform/tls_socket.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <array>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace umbra::detail::platform {
namespace {

[[nodiscard]] std::string opensslError(char const* operation) {
  std::string message(operation);
  auto const error = ERR_get_error();
  if (error != 0U) {
    std::array<char, 256U> detail{};
    ERR_error_string_n(error, detail.data(), detail.size());
    message += ": ";
    message += detail.data();
  } else {
    message += " failed without an OpenSSL diagnostic.";
  }
  return message;
}

void requireTls13(SSL_CTX* context) {
  if (SSL_CTX_set_min_proto_version(context, TLS1_3_VERSION) != 1 ||
      SSL_CTX_set_max_proto_version(context, TLS1_3_VERSION) != 1) {
    throw std::runtime_error(opensslError("Configuring TLS 1.3"));
  }
}

[[nodiscard]] std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>
createServerContext(TlsServerConfiguration const& configuration) {
  auto context = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>(
      SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
  if (!context) {
    throw std::runtime_error(opensslError("Creating the TLS server context"));
  }
  requireTls13(context.get());
  if (configuration.certificateChainFile.empty() ||
      configuration.privateKeyFile.empty() ||
      SSL_CTX_use_certificate_chain_file(
          context.get(),
          configuration.certificateChainFile.string().c_str()) != 1 ||
      SSL_CTX_use_PrivateKey_file(
          context.get(),
          configuration.privateKeyFile.string().c_str(),
          SSL_FILETYPE_PEM) != 1 ||
      SSL_CTX_check_private_key(context.get()) != 1) {
    throw std::runtime_error(opensslError("Loading the TLS server credentials"));
  }
  return context;
}

void attachSocket(SSL* session, NativeSocket socket) {
  if (socket == invalidSocket ||
      SSL_set_fd(session, static_cast<int>(socket)) != 1) {
    throw std::runtime_error(opensslError("Attaching the TLS session to its socket"));
  }
}

void performHandshake(
    SSL* session,
    NativeSocket socket,
    bool server,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  while (true) {
    auto const result = server ? SSL_accept(session) : SSL_connect(session);
    if (result == 1) {
      break;
    }
    auto const error = SSL_get_error(session, result);
    if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
      auto const waitForRead = error == SSL_ERROR_WANT_READ;
      auto const waitForWrite = error == SSL_ERROR_WANT_WRITE;
      if (deadline) {
        if (!waitForSocketActivityUntil(
                socket, waitForRead, waitForWrite, *deadline)) {
          throw std::runtime_error("The TLS 1.3 handshake exceeded its deadline.");
        }
      } else {
        waitForSocketActivity(socket, waitForRead, waitForWrite);
      }
      continue;
    }
    auto const operation = server ? "Accepting the TLS 1.3 session"
                                  : "Connecting the TLS 1.3 session";
    auto message = opensslError(operation);
    message += " (OpenSSL status ";
    message += std::to_string(error);
    message += ").";
    throw std::runtime_error(std::move(message));
  }
  if (SSL_version(session) != TLS1_3_VERSION) {
    throw std::runtime_error(
        "The TLS transport negotiated a protocol other than TLS 1.3.");
  }
}

void waitForTlsActivity(
    NativeSocket socket,
    bool waitForRead,
    bool waitForWrite,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  if (deadline) {
    if (!waitForSocketActivityUntil(
            socket, waitForRead, waitForWrite, *deadline)) {
      throw std::runtime_error("The TLS socket operation exceeded its deadline.");
    }
  } else {
    waitForSocketActivity(socket, waitForRead, waitForWrite);
  }
}

}  // namespace

struct TlsSocket::Impl final {
  class Operation final {
   public:
    explicit Operation(Impl& implementation) : implementation_(implementation) {
      implementation_.beginOperation();
    }
    Operation(Operation const&) = delete;
    Operation& operator=(Operation const&) = delete;
    ~Operation() { implementation_.endOperation(); }

   private:
    Impl& implementation_;
  };

  Impl(
      NativeSocket nativeSocket,
      SSL_CTX* sslContext,
      SSL* sslSession,
      int negotiatedVersion)
      : socket(nativeSocket),
        context(sslContext),
        session(sslSession),
        protocolVersion(negotiatedVersion) {}

  ~Impl() { close(); }

  void close() noexcept {
    NativeSocket socketToClose = invalidSocket;
    {
      std::unique_lock lifecycleLock(lifecycleMutex);
      if (resourcesClosed) {
        return;
      }
      if (closing) {
        lifecycleChanged.wait(lifecycleLock, [this] { return resourcesClosed; });
        return;
      }
      closing = true;
      socketToClose = socket;
    }

    // Wake nonblocking socket waits before waiting for active operations.
    interruptSocket(socketToClose);
    {
      std::unique_lock lifecycleLock(lifecycleMutex);
      lifecycleChanged.wait(
          lifecycleLock, [this] { return activeOperations == 0U; });
    }

    {
      std::scoped_lock ioLock(ioMutex);
      if (session != nullptr) {
        SSL_free(session);
        session = nullptr;
      }
      if (context != nullptr) {
        SSL_CTX_free(context);
        context = nullptr;
      }
    }
    if (socketToClose != invalidSocket) {
      closeSocket(socketToClose);
    }
    {
      std::scoped_lock lifecycleLock(lifecycleMutex);
      socket = invalidSocket;
      resourcesClosed = true;
    }
    lifecycleChanged.notify_all();
  }

  void beginOperation() {
    std::scoped_lock lifecycleLock(lifecycleMutex);
    if (closing || resourcesClosed || session == nullptr) {
      throw std::runtime_error("The TLS socket is closed.");
    }
    ++activeOperations;
  }

  void endOperation() noexcept {
    {
      std::scoped_lock lifecycleLock(lifecycleMutex);
      --activeOperations;
    }
    lifecycleChanged.notify_all();
  }

  void ensureOpen() const {
    std::scoped_lock lifecycleLock(lifecycleMutex);
    if (closing || resourcesClosed || session == nullptr) {
      throw std::runtime_error("The TLS socket is closed.");
    }
  }

  NativeSocket socket = invalidSocket;
  SSL_CTX* context = nullptr;
  SSL* session = nullptr;
  int protocolVersion = 0;
  mutable std::mutex lifecycleMutex;
  std::condition_variable lifecycleChanged;
  std::mutex ioMutex;
  std::size_t activeOperations = 0U;
  bool closing = false;
  bool resourcesClosed = false;
};

TlsSocket::TlsSocket(std::unique_ptr<Impl> implementation) noexcept
    : implementation_(std::move(implementation)) {}

TlsSocket::TlsSocket(TlsSocket&&) noexcept = default;
TlsSocket& TlsSocket::operator=(TlsSocket&&) noexcept = default;
TlsSocket::~TlsSocket() = default;

TlsSocket TlsSocket::connect(
    NativeSocket socket,
    TlsClientConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  auto* context = SSL_CTX_new(TLS_client_method());
  if (context == nullptr) {
    closeSocket(socket);
    throw std::runtime_error(opensslError("Creating the TLS client context"));
  }
  SSL* session = nullptr;
  try {
    requireTls13(context);
    switch (configuration.certificateValidationMode) {
      case TlsClientConfiguration::CertificateValidationMode::server_certificate:
        SSL_CTX_set_verify(context, SSL_VERIFY_PEER, nullptr);
        if (configuration.trustStoreFile.empty()) {
          if (SSL_CTX_set_default_verify_paths(context) != 1) {
            throw std::runtime_error(
                opensslError("Loading the system TLS trust store"));
          }
        } else if (SSL_CTX_load_verify_locations(
                       context,
                       configuration.trustStoreFile.string().c_str(),
                       nullptr) != 1) {
          throw std::runtime_error(
              opensslError("Loading the configured TLS trust store"));
        }
        break;
      case TlsClientConfiguration::CertificateValidationMode::encryption_only:
        SSL_CTX_set_verify(context, SSL_VERIFY_NONE, nullptr);
        break;
      default:
        throw std::runtime_error("The TLS certificate-validation mode is invalid.");
    }

    session = SSL_new(context);
    if (session == nullptr) {
      throw std::runtime_error(opensslError("Creating the TLS client session"));
    }
    if (configuration.peerName.empty() ||
        SSL_set_tlsext_host_name(session, configuration.peerName.c_str()) != 1 ||
        (configuration.certificateValidationMode ==
             TlsClientConfiguration::CertificateValidationMode::server_certificate &&
         SSL_set1_host(session, configuration.peerName.c_str()) != 1)) {
      throw std::runtime_error(
          "Configuring TLS server-name or certificate verification failed.");
    }
    setSocketNonBlocking(socket, true);
    attachSocket(session, socket);
    performHandshake(session, socket, false, deadline);
    auto implementation = std::make_unique<Impl>(
        socket, context, session, SSL_version(session));
    context = nullptr;
    session = nullptr;
    return TlsSocket(std::move(implementation));
  } catch (...) {
    if (session != nullptr) {
      SSL_free(session);
    }
    if (context != nullptr) {
      SSL_CTX_free(context);
    }
    closeSocket(socket);
    throw;
  }
}

TlsSocket TlsSocket::accept(
    NativeSocket socket,
    TlsServerConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> context(
      nullptr, SSL_CTX_free);
  SSL* session = nullptr;
  try {
    context = createServerContext(configuration);
    session = SSL_new(context.get());
    if (session == nullptr) {
      throw std::runtime_error(opensslError("Creating the TLS server session"));
    }
    setSocketNonBlocking(socket, true);
    attachSocket(session, socket);
    performHandshake(session, socket, true, deadline);
    auto implementation = std::make_unique<Impl>(
        socket, context.release(), session, SSL_version(session));
    session = nullptr;
    return TlsSocket(std::move(implementation));
  } catch (...) {
    if (session != nullptr) {
      SSL_free(session);
    }
    closeSocket(socket);
    throw;
  }
}

void TlsSocket::validateServerConfiguration(
    TlsServerConfiguration const& configuration) {
  static_cast<void>(createServerContext(configuration));
}

void TlsSocket::sendAll(std::span<std::uint8_t const> bytes) {
  if (!implementation_) {
    throw std::runtime_error("The TLS socket is closed.");
  }
  Impl::Operation activeOperation(*implementation_);
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    std::unique_lock ioLock(implementation_->ioMutex);
    implementation_->ensureOpen();
    auto const remaining = bytes.size() - offset;
    auto const chunkSize = std::min(
        remaining,
        static_cast<std::size_t>(std::numeric_limits<int>::max()));
    std::size_t written = 0U;
    auto const result = SSL_write_ex(
        implementation_->session,
        bytes.data() + offset,
        chunkSize,
        &written);
    if (result != 1 || written == 0U) {
      auto const error = SSL_get_error(implementation_->session, result);
      if (error == SSL_ERROR_WANT_READ) {
        ioLock.unlock();
        try {
          waitForSocketActivity(implementation_->socket, true, false);
        } catch (...) {
          implementation_->ensureOpen();
          throw;
        }
      } else if (error == SSL_ERROR_WANT_WRITE) {
        try {
          waitForSocketActivity(implementation_->socket, false, true);
        } catch (...) {
          implementation_->ensureOpen();
          throw;
        }
      } else {
        throw std::runtime_error(opensslError("Writing to the TLS socket"));
      }
      implementation_->ensureOpen();
      continue;
    }
    offset += written;
  }
}

ReceiveExactResult TlsSocket::receiveExact(
    std::span<std::uint8_t> bytes,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  if (!implementation_) {
    throw std::runtime_error("The TLS socket is closed.");
  }
  Impl::Operation activeOperation(*implementation_);
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    std::unique_lock ioLock(implementation_->ioMutex);
    implementation_->ensureOpen();
    auto const remaining = bytes.size() - offset;
    auto const chunkSize = std::min(
        remaining,
        static_cast<std::size_t>(std::numeric_limits<int>::max()));
    std::size_t received = 0U;
    auto const result = SSL_read_ex(
        implementation_->session,
        bytes.data() + offset,
        chunkSize,
        &received);
    if (result == 1 && received != 0U) {
      offset += received;
      continue;
    }
    auto const error = SSL_get_error(implementation_->session, result);
    if (error == SSL_ERROR_ZERO_RETURN) {
      return ReceiveExactResult::end_of_stream;
    }
    if (error == SSL_ERROR_WANT_READ) {
      ioLock.unlock();
      try {
        waitForTlsActivity(implementation_->socket, true, false, deadline);
      } catch (...) {
        implementation_->ensureOpen();
        throw;
      }
    } else if (error == SSL_ERROR_WANT_WRITE) {
      try {
        waitForTlsActivity(implementation_->socket, false, true, deadline);
      } catch (...) {
        implementation_->ensureOpen();
        throw;
      }
    } else {
      throw std::runtime_error(opensslError("Reading from the TLS socket"));
    }
    implementation_->ensureOpen();
  }
  return ReceiveExactResult::complete;
}

int TlsSocket::negotiatedProtocolVersion() const noexcept {
  return implementation_ != nullptr
      ? implementation_->protocolVersion
      : 0;
}

NativeSocket TlsSocket::nativeSocket() const noexcept {
  if (implementation_ == nullptr) {
    return invalidSocket;
  }
  std::scoped_lock lifecycleLock(implementation_->lifecycleMutex);
  return implementation_->resourcesClosed
      ? invalidSocket
      : implementation_->socket;
}

void TlsSocket::close() noexcept {
  if (implementation_) {
    implementation_->close();
  }
}

}  // namespace umbra::detail::platform
