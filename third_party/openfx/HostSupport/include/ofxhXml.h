

#ifndef OFX_XML_H
#define OFX_XML_H

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

namespace OFX {

  namespace XML {

    inline std::string escape(const std::string &s) {
      std::string ns;
      for (size_t i=0;i<s.size();i++) {
        // The are exactly five characters which must be escaped
        // http://www.w3.org/TR/xml/#syntax
        switch (s[i]) {
          case '<':
            ns += "&lt;";
            break;
          case '>':
            ns += "&gt;";
            break;
          case '&':
            ns += "&amp;";
            break;
          case '"':
            ns += "&quot;";
            break;
          case '\'':
            ns += "&apos;";
            break;
          default: {
            unsigned char c = (unsigned char)(s[i]);
            // Escape even the whitespace characters '\n' '\r' '\t', although they are valid
            // XML, because they would be converted to space when re-read.
            // See http://www.w3.org/TR/xml/#AVNormalize
            if ((0x01 <= c && c <= 0x1f) || (0x7F <= c && c <= 0x9F)) {
              // these characters must be escaped in XML 1.1
              // http://www.w3.org/TR/xml/#sec-references
              ns += "&#x";
              if (c > 0xf) {
                int d = c / 0x10;
                ns += d < 10 ? ('0' + d) : ('A' + d - 10);
              }
              int d = c & 0xf;
              ns += d < 10 ? ('0' + d) : ('A' + d - 10);
              ns += ';';
            } else {
              ns += s[i];
            }
          } break;
        }
      }
      return ns;
    }

    inline std::string attribute(const std::string &at, const std::string &val)
    {
      return at + "=" + "\"" + escape(val) + "\" ";
    }

    inline std::string attribute(const std::string &st, int val)
    {
      std::ostringstream o;
      o << val;
      return attribute(st, o.str());
    }

  }
}

#endif
