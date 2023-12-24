// acceptor.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------

#include "sockpp/acceptor.h"

#include <cstring>

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

result<acceptor> acceptor::create(int domain) noexcept {
    acceptor acc(create_handle(domain));
    if (!acc)
        return result<acceptor>::from_last_error();
    return acc;
}

// --------------------------------------------------------------------------

// This attempts to open the acceptor, bind to the requested address, and
// start listening. On any error it will be sure to leave the underlying
// socket in an unopened/invalid state.
// If the acceptor appears to already be opened, this will quietly succeed
// without doing anything.

result<> acceptor::open(
    const sock_address& addr, int queSize /*=DFLT_QUE_SIZE*/, bool reuseSock /*=true*/
) noexcept {
    // TODO: What to do if we are open but bound to a different address?
    if (is_open())
        return none{};

    sa_family_t domain = addr.family();
    socket_t h = create_handle(domain);

    if (h == INVALID_SOCKET)
        return result<>::from_last_error();

    reset(h);

#if defined(_WIN32) || defined(__CYGWIN__)
    const int REUSE = SO_REUSEADDR;
#else
    const int REUSE = SO_REUSEPORT;
#endif

    if (reuseSock && (domain == AF_INET || domain == AF_INET6)) {
        int reuse = 1;
        if (auto res = set_option(SOL_SOCKET, REUSE, reuse); !res) {
            close();
            return res;
        }
    }

    if (auto res = bind(addr); !res) {
        close();
        return res;
    }

    if (auto res = listen(queSize); !res) {
        close();
        return res;
    }

    return none{};
}

// --------------------------------------------------------------------------

result<stream_socket> acceptor::accept(sock_address* clientAddr /*=nullptr*/) noexcept {
    sockaddr* p = clientAddr ? clientAddr->sockaddr_ptr() : nullptr;
    socklen_t len = clientAddr ? clientAddr->size() : 0;

    if (auto res = check_socket(::accept(handle(), p, clientAddr ? &len : nullptr)); !res)
        return res.error();
    else
        return stream_socket{res.value()};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
