
/*
IPAddress.h - Base class that provides IPAddress
Copyright (c) 2011 Adrian McEwen.  All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef IPAddress_h
#define IPAddress_h

#include <stdint.h>

typedef  uint8_t ip_addr_t[4];


// to display a netif id with printf:
#define NETIFID_STR        "%c%c%u"
#define NETIFID_VAL(netif) \
        ((netif)? (netif)->name[0]: '-'),     \
        ((netif)? (netif)->name[1]: '-'),     \
        ((netif)? netif_get_index(netif): 42)

// A class to make it easier to handle and pass around IP addresses
// IPv6 update:
// IPAddress is now a decorator class for lwIP's ip_addr_t
// fully backward compatible with legacy IPv4-only Arduino's
// with unchanged footprint when IPv6 is disabled

class IPAddress  {
private:

    uint32_t  _ip;

    inline uint8_t* raw_address() {
        uint8_t* addr = reinterpret_cast<uint8_t*>(&_ip);
        return addr;
    }
public:
    // Constructors
    IPAddress();
    IPAddress(const IPAddress& from);
    IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
    IPAddress(int address);
    IPAddress(const uint8_t *address);

    

    
    uint8_t& operator[](int index);
    
    // Overloaded copy operators to allow initialisation of IPAddress objects from other types

    IPAddress& operator=(const IPAddress&) = default;          
};


#endif
