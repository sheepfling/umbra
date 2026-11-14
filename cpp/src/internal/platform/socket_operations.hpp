#pragma once

#include <cstdint>
#include <chrono>
#include <span>
#include <string>
#include <string_view>

namespace umbra::detail::platform {

using NativeSocket = std::uintptr_t;
inline constexpr NativeSocket invalidSocket = static_cast<NativeSocket>(-1);

enum class ReceiveExactResult { complete, end_of_stream };

void initializeSocketRuntime();
void closeSocket(NativeSocket socket) noexcept;
void interruptSocket(NativeSocket socket) noexcept;
void setSocketNonBlocking(NativeSocket socket, bool nonBlocking);
void waitForSocketActivity(
    NativeSocket socket,
    bool waitForRead,
    bool waitForWrite);
[[nodiscard]] bool waitForSocketActivityUntil(
    NativeSocket socket,
    bool waitForRead,
    bool waitForWrite,
    std::chrono::steady_clock::time_point deadline);

[[nodiscard]] NativeSocket connectSocket(
    std::string_view host,
    std::uint16_t port);
[[nodiscard]] NativeSocket connectSocket(
    std::string_view host,
    std::uint16_t port,
    std::chrono::steady_clock::time_point deadline);
[[nodiscard]] NativeSocket listenSocket(
    std::string_view host,
    std::uint16_t requestedPort,
    std::uint16_t& boundPort);
[[nodiscard]] NativeSocket acceptSocket(NativeSocket listener);
[[nodiscard]] std::string peerAddress(NativeSocket socket);

void sendAll(NativeSocket socket, std::span<std::uint8_t const> bytes);
[[nodiscard]] ReceiveExactResult receiveExact(
    NativeSocket socket,
    std::span<std::uint8_t> bytes);
[[nodiscard]] ReceiveExactResult receiveExact(
    NativeSocket socket,
    std::span<std::uint8_t> bytes,
    std::chrono::steady_clock::time_point deadline);

}  // namespace umbra::detail::platform
