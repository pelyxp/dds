/**
 *    Copyright (C) 2016 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <string>
#include <vector>

#ifndef _WIN32

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#ifdef __OpenBSD__
#include <sys/uio.h>
#endif  // __OpenBSD__

#endif  // not _WIN32

#include "mongo/base/string_data.h"

namespace mongo {

#if defined(_WIN32)

typedef short sa_family_t;
typedef int socklen_t;

// This won't actually be used on windows
struct sockaddr_un {
    short sun_family;
    char sun_path[108];  // length from unix header
};

#endif  // _WIN32

// Generate a string representation for getaddrinfo return codes
std::string getAddrInfoStrError(int code);

/**
 * Wrapper around os representation of network address.
 */
struct SockAddr {
    SockAddr();

    explicit SockAddr(int sourcePort); /* listener side */

    /**
     * Initialize a SockAddr for a given IP or Hostname.
     *
     * If target fails to resolve/parse, SockAddr.isValid() may return false,
     * or the resulting SockAddr may be equivalent to SockAddr(port).
     *
     * If target is a unix domain socket, a uassert() exception will be thrown
     * on windows or if addr exceeds maximum path length.
     *
     * If target resolves to more than one address, only the first address
     * will be used. Others will be discarded.
     * SockAddr::createAll() is recommended for capturing all addresses.
     */
    explicit SockAddr(StringData target, int port, sa_family_t familyHint);

    explicit SockAddr(const sockaddr_storage& other, socklen_t size);

    /**
     * Resolve an ip or hostname to a vector of SockAddr objects.
     *
     * Works similar to SockAddr(StringData, int, sa_family_t) above,
     * however all addresses returned from ::getaddrinfo() are used,
     * it never falls-open to SockAddr(port),
     * and isInvalid() SockAddrs are excluded.
     *
     * May return an empty vector.
     */
    static std::vector<SockAddr> createAll(StringData target, int port, sa_family_t familyHint);

    template <typename T>
    T& as() {
        return *(T*)(&sa);
    }
    template <typename T>
    const T& as() const {
        return *(const T*)(&sa);
    }

    std::string hostOrIp() const {
        return _hostOrIp;
    }

    std::string toString(bool includePort = true) const;

    bool isValid() const {
        return _isValid;
    }

    bool isIP() const;

    /**
     * @return one of AF_INET, AF_INET6, or AF_UNIX
     */
    sa_family_t getType() const;

    unsigned getPort() const;

    std::string getAddr() const;

    bool isLocalHost() const;
    bool isDefaultRoute() const;
    bool isAnonymousUNIXSocket() const;

    bool operator==(const SockAddr& r) const;
    bool operator!=(const SockAddr& r) const;
    bool operator<(const SockAddr& r) const;

    const sockaddr* raw() const {
        return (sockaddr*)&sa;
    }
    sockaddr* raw() {
        return (sockaddr*)&sa;
    }

    socklen_t addressSize;

private:
    void initUnixDomainSocket(const std::string& path, int port);

    std::string _hostOrIp;
    struct sockaddr_storage sa;
    bool _isValid;
};

}  // namespace mongo
