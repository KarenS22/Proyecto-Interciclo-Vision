#ifndef BASE64_HPP
#define BASE64_HPP

#include <vector>
#include <string>
#include <cstddef>

// Funciones de codificación/decodificación Base64
std::string base64_encode(const unsigned char* buf, unsigned int bufLen);
std::vector<unsigned char> base64_decode(std::string const& encoded_string);

#endif // BASE64_HPP

