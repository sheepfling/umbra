#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace umbra::detail::platform {

using NativeSocket = std::uintptr_t;
inline constexpr NativeSocket invalidSocket = static_cast<NativeSocket>(-1);

enum class ReceiveExactResult { complete, end_of_stream };

void initializeSocketRuntime();
void closeSocket(NativeSocket socket) noexcept;

[[nodiscard]] NativeSocket connectSocket(
    std::string_view host,
    std::uint16_t port);
[[nodiscard]] NativeSocket listenSocket(
    std::string_view host,
    std::uint16_t requestedPort,
    std::uint16_t& boundPort);
[[nodiscard]] NativeSocket acceptSocket(NativeSocket listener);

void sendAll(NativeSocket socket, std::span<std::uint8_t const> bytes);
[[nodiscard]] ReceiveExactResult receiveExact(
    NativeSocket socket,
    std::span<std::uint8_t> bytes);

}  // namespace umbra::detail::platform
