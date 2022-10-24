/*!
 * idprom.cc
 *
 * Copyright (c) 2021-2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <date/date.h>
#include <date/iso_week.h>

#include <bsp/fwd.h>
#include <bsp/idprom.h>

#include <private/sysfs.h>

namespace bsp2 {

namespace fs = std::filesystem;

class eofException : public std::ios_base::failure
{
    public:
        explicit eofException(const std::string &message)
              : failure(message)
        {
        }
};

class idprom_t::descriptor {
    public:
        descriptor(const fs::path &path,
                   std::streamsize offset, std::streamsize size)
            : m_path(path)
            , m_size(size)
            , m_block_size(size ? size : 128)
            , m_offset(0)
            , m_read_size(0)
            , m_at_eof(false)
        {
            bool guard(path.string().find("/w1/") != std::string::npos);
            m_f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            m_f.open(path, std::ifstream::binary);
            m_f.seekg(0, m_f.end);
            auto length = m_f.tellg();
            if (length < 0) {
                length = offset;
                m_at_eof = true;
            } else {
                if (((offset + m_size) > length) || !m_size) {
                    m_size = (size_t)length - (size_t)offset;
                }

                m_f.seekg(offset, m_f.beg);
                m_data.resize(m_size);
                // Check if path is a symlink with /w1/
                if (!guard && fs::exists(m_path) && fs::is_symlink(m_path)) {
                    std::error_code ec;
                    auto symlink_path = fs::read_symlink(m_path, ec);
                    guard = !ec && (symlink_path.string().find("/w1/")
                                                != std::string::npos);
                }
                if (guard) {
                    static std::mutex m;
                    std::lock_guard<std::mutex> l(m);
                    // Read the entire w1 idprom under a lock
                    fetch(m_size);
                } else {
                    fetch(std::min(m_block_size, m_size));
                }
            }
        }

        fs::path m_path;
        std::streamsize m_size;
        std::streamsize m_block_size;
        std::streamsize m_offset;
        std::streamsize m_read_size;
        bool m_at_eof;
        std::ifstream m_f;

        std::vector<uint8_t> m_data;

        void fetch(std::streamsize bytes)
        {
            while (!m_at_eof && (m_read_size < bytes)) {
                std::streamsize remaining(m_data.size() - (m_offset + m_read_size));
                auto rbytes = std::min(m_block_size, remaining);
                if (rbytes) {
                    char *s = reinterpret_cast<char *>(m_data.data() + m_offset + m_read_size);
                    m_f.read(s, rbytes);
                    m_read_size += m_f.gcount();
                }
                if (!rbytes || (m_f.gcount() != rbytes) || m_f.eof()) {
                    m_at_eof = true;
                    m_f.close();
                }
            }
        }

        void skip(std::streamsize bytes)
        {
            if (m_read_size < bytes) {
                fetch(bytes);
                if (m_read_size < bytes) {
                    std::ostringstream v;
                    v <<  m_path
                      << ": end of file reading "
                      << bytes
                      << " bytes at offset "
                      << m_offset
                      ;;
                    throw eofException(v.str());
                }
            }
            m_offset += bytes;
            m_read_size -= bytes;
        }

        bool eof(std::streamsize bytes=4)
        {
            if (m_read_size < bytes) {
                fetch(bytes);
            }
            return (m_read_size < bytes);
        }

        std::pair<bool,uint8_t> rd_byte()
        {
            bool b = false;
            uint8_t c = 0;
            if (!eof(1)) {
                c = m_data[m_offset];
                b = true;
                skip(1);
            }
            return std::pair<bool,uint8_t>(b,c);
        }

        std::pair<bool,uint16_t> rd_word()
        {
            bool b = false;
            uint16_t w = 0;
            if (!eof(2)) {
                w = (m_data[m_offset + 0] << 8)
                  |  m_data[m_offset + 1]
                  ;
                b = true;
                skip(2);
            }
            return std::pair<bool,uint16_t>(b,w);
        }

        std::pair<bool,uint32_t> rd_dword()
        {
            bool b = false;
            uint32_t w = 0;
            if (!eof(4)) {
                w = (m_data[m_offset + 0] << 24)
                  | (m_data[m_offset + 1] << 16)
                  | (m_data[m_offset + 2] <<  8)
                  |  m_data[m_offset + 3]
                  ;
                skip(4);
            }
            return std::pair<bool,uint32_t>(b,w);
        }

        std::vector<uint8_t> value(std::streamsize size)
        {
            if (eof(size)) {
                std::ostringstream v;
                v <<  m_path
                  << ": end of file reading value of len "
                  << size
                  << " bytes at offset "
                  << m_offset
                  ;;
                throw eofException(v.str());
            }
            auto it = m_data.begin() + m_offset;
            skip(size);
            return std::vector<uint8_t>(it, it + size);
        }
        std::pair<bool,std::string> rd_str(size_t bytes)
        {
            if (!eof(bytes)) {
                std::string s(m_data.begin() + m_offset,
                              m_data.begin() + m_offset + bytes);
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                            return !!(ch);
                        }).base(), s.end());
                skip(bytes);
                return std::pair<bool,std::string>(true,s);
            } else {
                return std::pair<bool,std::string>(false,"");
            }
        }
};

class idprom_t::tlv {
    private:
        const size_t T_IDPROM_EXTENSION = 0;
        const size_t T_IDPROM_EXT_OFFSET = 0x100;  // Extension marker adds offset to tag
        enum e_fmt {
           hex,
           decimal,
           ascii,
           reserved,
           assy_pn_4,
           assy_pn_5,
           assy_pn_6,
           pcb_partnbr_4,
           pcb_partnbr_6,
           hw_version,
           _size,
        };

        static std::string
        parse_hex(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;
            for (auto i : data) {
                v << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(i);
            }
            return v.str();
        }
        static std::string
        parse_decimal(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;
            if (data.size() == 1) {
                v << (unsigned int)data[0];
            } else if (data.size() == 2) {
                size_t s = (data[0] << 8)
                         |  data[1];
                v << s;
            } else if (data.size() == 4) {
                size_t s = (data[0] << 24)
                         | (data[1] << 16)
                         | (data[2] << 8)
                         |  data[3];
                v << s;
            } else if (data.size() == 8) {
                size_t h = (data[0] << 24)
                         | (data[1] << 16)
                         | (data[2] << 8)
                         |  data[3];
                size_t l = (data[4] << 24)
                         | (data[5] << 16)
                         | (data[6] << 8)
                         |  data[7];
                unsigned long long s =
                    ((unsigned long long)h << 32ul) | l;
                v << s;
            } else {
                std::string s = "0x";
                s.append(parse_hex(data));
                v << std::stoul(s);
            }
            return v.str();
        }
        static std::string
        parse_ascii(const std::vector<uint8_t> &data)
        {
            std::string s(data.begin(), data.end());
            auto it = std::find_if(s.rbegin(), s.rend(),
                            [](unsigned char ch) {
                                return !(!ch || ch == ' ' || ch == '\t');
                            });
            s.erase(it.base(), s.end());
            return std::regex_replace(s, std::regex("^0+(\\d+)$"), "$1");
        }
        static std::string
        parse_reserved(const std::vector<uint8_t> &data)
        {
            return std::string(data.begin(), data.end());
        }
        static std::string
        parse_assy_pn_4(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 4) {
                size_t class_code = data[0];
                size_t base = (data[1] << 8)
                            |  data[2]
                            ;
                size_t version = data[3];

                v << std::setfill('0') << std::setw(2) << class_code << "-"
                  << std::setfill('0') << std::setw(4) << base << "-"
                  << std::setfill('0') << std::setw(2) << version
                  ;
            }
            return v.str();
        }
        static std::string
        parse_assy_pn_5(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 5) {
                size_t class_code = (data[0] << 8)
                                  |  data[1]
                                  ;
                size_t base = (data[2] << 8)
                            |  data[3]
                            ;
                size_t version = data[4];

                v << std::setfill('0') << std::setw(3) << class_code << "-"
                  << std::setfill('0') << std::setw(4) << base << "-"
                  << std::setfill('0') << std::setw(2) << version
                  ;
            }
            return v.str();
        }
        static std::string
        parse_assy_pn_6(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 6) {
                size_t class_code = (data[0] << 8)
                                  |  data[1]
                                  ;
                size_t base = (data[2] << 16)
                            | (data[3] << 8)
                            |  data[4]
                            ;
                size_t version = data[5];

                v << std::setfill('0') << std::setw(3) << class_code << "-"
                  << std::setfill('0') << std::setw(5) << base << "-"
                  << std::setfill('0') << std::setw(2) << version
                  ;
            }
            return v.str();
        }
        static std::string
        parse_pcb_partnbr_4(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 4) {
                size_t class_code = data[0];
                size_t base = (data[1] << 8)
                            |  data[2]
                            ;
                size_t version = data[3];

                v << std::setfill('0') << std::setw(2) << class_code << "-"
                  << std::setfill('0') << std::setw(4) << base << "-"
                  << std::setfill('0') << std::setw(2) << version
                  ;
            }
            return v.str();
        }
        static std::string
        parse_pcb_partnbr_6(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 6) {
                size_t class_code = (data[0] << 8)
                                  |  data[1]
                                  ;
                size_t base = (data[2] << 16)
                            | (data[3] << 8)
                            |  data[4]
                            ;
                size_t version = data[5];

                v << std::setfill('0') << std::setw(3) << class_code << "-"
                  << std::setfill('0') << std::setw(5) << base << "-"
                  << std::setfill('0') << std::setw(2) << version
                  ;
            }
            return v.str();
        }
        static std::string
        parse_hw_version(const std::vector<uint8_t> &data)
        {
            std::ostringstream v;

            if (data.size() == 2) {
                size_t major = data[0];
                size_t minor = data[1];

                v << major << "." << minor;
            }
            return v.str();
        }

        typedef std::string (*parse_fn)(const std::vector<uint8_t> &data);

        static parse_fn parse_fn_map[e_fmt::_size];
        static const std::map<std::string, size_t> name_to_tag_map;
        static std::map<size_t, std::string> tag_to_name_map;
        static const std::map<std::string, e_fmt> fmt_override;

        static std::once_flag once_inited;

        e_fmt fmt(e_fmt f) const
        {
            auto it = fmt_override.find(m_name);
            if (it != fmt_override.end()) {
                return it->second;
            }
            return f;
        }

        void set_value(const std::vector<uint8_t> &data, e_fmt f)
        {
            if (static_cast<int>(f) >= static_cast<int>(e_fmt::_size)) {
                m_value = parse_reserved(data);
            } else {
                m_value = parse_fn_map[f](data);
            }
        }
        void set_value(const std::string &s)
        {
            m_value = s;
        }
        void set_tag(size_t tag)
        {
            m_tag = tag;
            auto it = tag_to_name_map.find(tag);
            if (it == tag_to_name_map.end()) {
                std::ostringstream v;
                v << "tag 0x" << std::hex << tag;
                m_name = v.str();
            } else {
                m_name = it->second;
            }
        }
        static void init_statics();

        size_t m_tag;
        std::string m_name;
        std::string m_value;

    public:
        tlv(size_t offset, size_t tag=0, const std::string &v="")
        {
            std::call_once(once_inited, init_statics);
            set_tag(tag);
            set_value(v);
        }
        tlv(descriptor &info, size_t tag=0, const std::string &v="")
           : m_tag(tag)
           , m_value(v)
        {
            std::call_once(once_inited, init_statics);

            read(info);
        }

        tlv() = delete;
        const std::string &name() const { return m_name; }
        const std::string &value() const { return m_value; }
        size_t tag() const { return m_tag; }
        static size_t code_map(const std::string &);

        void read(descriptor &info)
        {
            size_t tag = 0;
            auto p = info.rd_byte();
            while (p.first && (p.second == T_IDPROM_EXTENSION)) {
                tag += T_IDPROM_EXT_OFFSET;
                p = info.rd_byte();
            }
            if (p.first) {
                tag |= p.second;
            }
            set_tag(tag);

            // EOF marker
            if ((tag == 0xff) || !p.first || !p.second) {
                set_tag(0xff);
                return;
            }

            if (info.eof(2)) {
                throw eofException("EOF reading tag length");
            }

            auto f = fmt(e_fmt::decimal);
            if (m_tag < 0x40) {
                set_value(info.value(1), f);
            } else if (m_tag < 0x80) {
                set_value(info.value(2), f);
            } else if (m_tag < 0xc0) {
                set_value(info.value(4), f);
            } else if (m_tag < 0xf0) {
                auto v = info.rd_byte().second;
                auto size = v & 0x3f;
                f = e_fmt((v & 0xc0) >> 6);
                set_value(info.value(size), fmt(f));
            } else {
                auto v = info.rd_word().second;
                auto size = v & 0x3fff;
                f = e_fmt((v & 0xc000) >> 14);
                set_value(info.value(size), fmt(f));
            }
        }
};

const std::map<std::string, size_t> idprom_t::tlv::name_to_tag_map = {
   {"EXTENSION",        0x00}, // Extension marker
   {"NUM_SLOTS",        0x01}, // Number of slots in chassis
   {"FAB_VERSION",      0x02}, // Fab Version
   {"RMA_FAILCODE",     0x03}, // RMA test history/failure code
   {"RMA_HISTORY",      0x04}, // RMA history
   {"CONNECTOR_TYPE",   0x05}, // Card connector type
   {"EHSA_PREF_MSTR",   0x06}, // EHSA Preferred Master
   {"VENDOR",           0x07}, // Vendor specific identifier
   {"PROCESSOR",        0x09}, // Processor type identifier
   {"PS_TYPE",          0x0B}, // Power Supply Type
   {"BURST_CAL_REC",    0x0C}, // Burst Mode Calibration Rec
   {"FORMAT_REV",       0x0D}, // IDPROM Format Revision
   {"RACK_ID",          0x0E}, // Rack ID Number

   {"CONTROLLER_TYPE",  0x40}, // Controller type
   {"HW_VERSION",       0x41}, // HW version
   {"PCB_REVISION",     0x42}, // PCB Revision number
   {"MAC_BLKSIZE",      0x43}, // MAC address block size
   {"CAPABILITY",       0x44}, // Capability code
   {"SELFTEST_RSLT",    0x45}, // Self test result
   {"BOOT_TIMOUT",      0x46}, // Boot Time Out (in ms)
   {"MB_CHANNEL_ID",    0x47}, // Motherboard Channel ID
   {"GROUP_TYPE",       0x48}, // Group Type
   {"CLI_WRITE",        0x49}, // CLI Write Enable
   {"RADIO_COUNTRY_CODE",  0x4A}, // Radio Country Code

   {"DEVIATION",        0x80}, // Deviation Number
   {"RMA_NUMBER",       0x81}, // RMA number
   {"PCB_PARTNBR_4",    0x82}, // PCB part number (4-byte,73-)
   {"DATECODE",         0x83}, // Hardware date code
   {"MFG_ENGINEER",     0x84}, // Manufacturing Engineer
   {"FAB_PARTNBR_4",    0x85}, // Fab Part number (4-byte,28-)
   {"TUNER_TYPE",       0x86}, // Generic Tuner Type
   {"TOP_ASSY_PN_4",    0x87}, // Top Assy Number (4-byte,68-)
   {"NEW_DEVIATION",    0x88}, // New Deviation Number
   {"VERSION_ID",       0x89}, // Version Identifier (VID)
   {"PCB_REVISION_4",   0x8A}, // PCB Revision four bytes
   {"LICENSE_TID",      0x8B}, // Licensing Transaction ID
   {"FW_VERSION",       0x8C}, // Firmware Version
   {"TAN_REVISION",     0x8D}, // TAN Revision Number
   {"ECI",              0x8E}, // Equipment Catalog Item number

   {"TOP_ASSY_PN_6",    0xC0}, // Top Assy Number (6-byte,800-)
   {"PCB_SERIAL",       0xC1}, // PCB serial number
   {"CHASSIS_SERIAL",   0xC2}, // Chassis serial number
   {"MACADDR",          0xC3}, // Chassis base MAC address
   {"MFG_TEST",         0xC4}, // MFG Test Engineering field
   {"FIELD_DIAGS",      0xC5}, // Field Diagnostics results
   {"CLEI",             0xC6}, // CLEI Code
   {"ENVMON",           0xC7}, // Environmental Monitor data
   {"CALIBRATION",      0xC8}, // Calibration data
   {"DEVICE_VALS",      0xC9}, // Platform features
   {"THIRD_PARTY_PN",   0xCA}, // Third party part number
   {"PRODUCT_ID",       0xCB}, // PID (was Model String)
   {"ASSETID",          0xCC}, // AssetId field (1-31 bytes)
   {"CALIBRATION_2",    0xCD}, // Additional Calibration data
   {"MB_SERIAL",        0xCE}, // Motherboard Serial Number
   {"MACADDR_BASE",     0xCF}, // Base MAC address
   {"CARD_NAME",        0xD0}, // Card Name
   {"FILE_NAME",        0xD1}, // File Name
   {"ENCRYPTION_DIG",   0xD2}, // Encryption Digest for cards
   {"LONGIT_CALIB",     0xD3}, // Longitudinal Calibration
   {"ASSET_ALIAS",      0xD4}, // Asset Alias Name (1-31 bytes)
   {"PROCESSOR_LABEL",  0xD5}, // Processor Label identifier
   {"SYSTEM_CLOCK",     0xD6}, // System Clock Freq - CPU/Bus (Mhz)
   {"PWR_CONSUMPTION",  0xD7}, // Power Consumption (10mW units)
   {"RESERVED",         0xD8}, // Reserved TLV
   {"SIGNATURE_LIST",   0xD9}, // Signature list
   {"UDI_DESC",         0xDA}, // UDI Product Description
   {"UDI_NAME",         0xDB}, // UDI Product Name
   {"TOP_ASSY_PN_5",    0xDF}, // Top Assy Number (5-byte,341-)

   {"PCB_PARTNBR_6",    0xE2}, // PCB part number (6-byte,73-)
   {"FAB_PARTNBR_6",    0xE3}, // Fab Part number (6-byte,28-)
   {"ECI_NUMBER",       0xEB}, // Equipment Catalog Item ascii

   {"EXTD_ENVMON",      0xF3}, // Extended Environmental Monitor
   {"FIELD_DIAG_EXT",   0xF4}  // Field Diags Extended Info
};

const std::map<std::string, idprom_t::tlv::e_fmt> idprom_t::tlv::fmt_override = {
   {"PCB_SERIAL",       idprom_t::tlv::e_fmt::ascii},
   {"CHASSIS_SERIAL",   idprom_t::tlv::e_fmt::ascii},
   {"PRODUCT_ID",       idprom_t::tlv::e_fmt::ascii},
   {"ASSETID",          idprom_t::tlv::e_fmt::ascii},
   {"CARD_NAME",        idprom_t::tlv::e_fmt::ascii},
   {"FILE_NAME",        idprom_t::tlv::e_fmt::ascii},
   {"ASSET_ALIAS",      idprom_t::tlv::e_fmt::ascii},
   {"PROCESSOR_LABEL",  idprom_t::tlv::e_fmt::ascii},
   {"UDI_DESC",         idprom_t::tlv::e_fmt::ascii},
   {"UDI_NAME",         idprom_t::tlv::e_fmt::ascii},
   {"FAB_PARTNBR_6",    idprom_t::tlv::e_fmt::ascii},
   {"ECI_NUMBER",       idprom_t::tlv::e_fmt::ascii},
   {"VERSION_ID",       idprom_t::tlv::e_fmt::ascii},
   {"PCB_REVISION",     idprom_t::tlv::e_fmt::ascii},
   {"PCB_REVISION_4",   idprom_t::tlv::e_fmt::ascii},
   {"TOP_ASSY_PN_4",    idprom_t::tlv::e_fmt::assy_pn_4},
   {"TOP_ASSY_PN_5",    idprom_t::tlv::e_fmt::assy_pn_5},
   {"TOP_ASSY_PN_6",    idprom_t::tlv::e_fmt::assy_pn_6},
   {"PCB_PARTNBR_4",    idprom_t::tlv::e_fmt::pcb_partnbr_4},
   {"PCB_PARTNBR_6",    idprom_t::tlv::e_fmt::pcb_partnbr_6},
   {"PWR_CONSUMPTION",  idprom_t::tlv::e_fmt::decimal},
   {"DEVICE_VALS",      idprom_t::tlv::e_fmt::decimal},
   {"TAN_REVISION",     idprom_t::tlv::e_fmt::ascii},
   {"MFG_TEST",         idprom_t::tlv::e_fmt::ascii},
   {"HW_VERSION",       idprom_t::tlv::e_fmt::hw_version}
};

idprom_t::tlv::parse_fn idprom_t::tlv::parse_fn_map[e_fmt::_size];
std::map<size_t, std::string> idprom_t::tlv::tag_to_name_map;
std::once_flag idprom_t::tlv::once_inited;

void
idprom_t::tlv::init_statics()
{
    parse_fn_map[e_fmt::hex] = parse_hex;
    parse_fn_map[e_fmt::decimal] = parse_decimal;
    parse_fn_map[e_fmt::ascii] = parse_ascii;
    parse_fn_map[e_fmt::reserved] = parse_reserved;
    parse_fn_map[e_fmt::assy_pn_4] = parse_assy_pn_4;
    parse_fn_map[e_fmt::assy_pn_5] = parse_assy_pn_5;
    parse_fn_map[e_fmt::assy_pn_6] = parse_assy_pn_6;
    parse_fn_map[e_fmt::pcb_partnbr_4] = parse_pcb_partnbr_4;
    parse_fn_map[e_fmt::pcb_partnbr_6] = parse_pcb_partnbr_6;
    parse_fn_map[e_fmt::hw_version] = parse_hw_version;

    for (auto const &it : name_to_tag_map) {
         tag_to_name_map[it.second] = it.first;
    }
}

void
idprom_t::tlv_parse(idprom_t::descriptor &info)
{
    m_decoded.clear();
    try {
        info.skip(TLV_IDPROM_HEADER_SIZE);
        while (!info.eof(1)) {
            tlv t(info);
            if (t.tag() == 0xff) {
                break;
            }
            m_decoded[t.name()].push_back(t.value());
        }
    } catch (const eofException &e) {
        if (false) {
            std::cerr << e.what() << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

template<class C>
void idprom_t::dc_set(const char *k, const C &value_pair)
{
    if (value_pair.first) {
        m_decoded[k].push_back(std::to_string(value_pair.second));
    } else {
        std::ostringstream v;
        v <<  m_path
          << ": dc_set("
          << k
          << ")"
          ;;
        throw eofException(v.str());
    }
}

template<>
void idprom_t::dc_set(const char *k, const std::pair<bool,std::string> &value_pair)
{
    if (value_pair.first) {
        m_decoded[k].push_back(value_pair.second);
    } else {
        std::ostringstream v;
        v <<  m_path
          << ": dc_set("
          << k
          << ")"
          ;;
        throw eofException(v.str());
    }
}

void
idprom_t::dc_parse(idprom_t::descriptor &info)
{

    m_decoded.clear();
    try {
        dc_set("DC_SIGNATURE", info.rd_word());
        dc_set("DC_VERSION", info.rd_byte());
        dc_set("DC_LENGTH", info.rd_byte());
        dc_set("DC_CHECKSUM", info.rd_word());
        dc_set("DC_SPROM_SIZE", info.rd_word());
        dc_set("DC_BLOCK_COUNT", info.rd_word());
        dc_set("DC_FRU_MAJOR", info.rd_word());
        dc_set("DC_FRU_MINOR", info.rd_word());
        dc_set("DC_OEM", info.rd_str(20));
        dc_set("PRODUCT_ID", info.rd_str(20));
        dc_set("PCB_SERIAL", info.rd_str(20));
        dc_set("PART_NUMBER", info.rd_str(16));
        dc_set("PART_REVISION", info.rd_str(4));
        dc_set("MFG_DEVIATION", info.rd_str(20));
        dc_set("DC_HW_REV_MAJOR", info.rd_word());
        dc_set("DC_HW_REV_MINOR", info.rd_word());
        dc_set("DC_MFG_BITS", info.rd_word());
        dc_set("DC_ENG_BITS", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("DC_SNMP_OID", info.rd_word());
        dc_set("PWR_CONSUMPTION", info.rd_word());
        dc_set("RMA_FAILCODE", info.rd_byte());
        dc_set("RMA_FAILCODE", info.rd_byte());
        dc_set("RMA_FAILCODE", info.rd_byte());
        dc_set("RMA_FAILCODE", info.rd_byte());
        dc_set("CLEI", info.rd_str(12));
        dc_set("VID", info.rd_str(4));

        auto board_id = (std::stoul(m_decoded["DC_FRU_MAJOR"][0]) << 16)
                      |  std::stoul(m_decoded["DC_FRU_MINOR"][0])
                      ;
        // Should this be in hex?
        m_decoded["DC_BOARD_ID"].push_back(std::to_string(board_id));

        auto hw_version(m_decoded["DC_HW_REV_MAJOR"][0]);
        hw_version.append(".").append(m_decoded["DC_HW_REV_MINOR"][0]);
        m_decoded["HW_VERSION"].push_back(hw_version);

        // Special handling
        auto signature = std::stoul(m_decoded["DC_SIGNATURE"][0]);
        if (signature != 0xabab) {
            std::ostringstream v;
            v <<  "Unexpected dc signature 0x"
              << std::hex << signature
              ;;
            throw std::domain_error(v.str());
        }

        auto dc_version = std::stoul(m_decoded["DC_VERSION"][0]);
        if (dc_version > 2) {
            // Change it to N/A if not displayable
            // if not m_decoded["VID"][0].value.isascii():
            if (!std::isprint(m_decoded["VID"][0].at(0))) {
                m_decoded["VID"][0] = "N/A";
            }
        } else {
            m_decoded["VID"][0] = std::string("V").append(m_decoded["RMA_FAILCODE"][2])
                                          .append(".")
                                          .append(m_decoded["RMA_FAILCODE"][3]);
        }
    } catch (const eofException &e) {
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

const std::map<std::string,std::vector<std::string>> &
idprom_t::read(bool refresh)
{
    std::lock_guard l(m_lock);
    if (refresh) {
        m_decoded.clear();
    }
    if (!m_decoded.size()) {
        try {
            /*
             * For w1 fallback, read the idprom only if we detect an idprom
             */
            bool parsed = true;
            if (!fallback_present()) {
                parsed = false;
                fallback();
            } else {
                descriptor info(path(true), m_offset, m_size);
                parse(info);
            }
            /*
             * In some cases, we might have missed the w1 reset
             * because of fan tray exchange.  In that case, we might
             * have previously shown fallback_present(), because
             * it was set from the previous fan tray.  However, if
             * there really is no idprom, then the parse will fail
             * and we should check fallback_present again, since the
             * w1 reset should have occurred during the attempted
             * read.
             */
            if (parsed && !m_decoded.size() && !fallback_present()) {
                fallback();
            }
            add_computed_fields();
        } catch (const std::exception &e) {
//          std::cerr << e.what() << std::endl;
        }
    }
    return m_decoded;
}

void
idprom_t::parse(descriptor &info)
{
    if (m_format == "tlv") {
        tlv_parse(info);
    } else if (m_format == "data_center") {
        dc_parse(info);
    } else {
        throw std::domain_error(m_format);
    }
}

void
idprom_t::fallback()
{
    if (m_fallback_algorithm == "w1") {
        if (fallback_present()) {
//          std::cerr << object_t(*this) << ": idprom empty, but presence detected" << std::endl;
            return;
        }
        if (m_fallback_status.empty()) {
//          std::cerr << object_t(*this) << ": no w1 status" << std::endl;
            return;
        }
        auto status = sysfs(m_fallback_status).get_value();
//      std::cerr << object_t(*this) << ": found w1 status " << status << std::endl;

        std::string direction;
        if (status.find("LL") != status.npos) {
            direction = "PORT-SIDE-INTAKE";
        } else {
            direction = "PORT-SIDE-EXHAUST";
        }
        auto content_it = m_fallback.find(direction);
        if (content_it == m_fallback.end()) {
            content_it = m_fallback.find("default");
            if (content_it == m_fallback.end()) {
//              std::cerr << object_t(*this) << ": idprom empty, no fallback map" << std::endl;
                return;
            }
        }
        for (const auto &it : content_it->second) {
            m_decoded[it.first].push_back(it.second);
        }
//      std::cerr << object_t(*this) << ": idprom empty"
//                << ", direction " << direction
//                << ", using content map " << content_name_it->second
//                << " as fallback"
//                << std::endl;
        return;
    }
}

idprom_t::idprom_t()
    : m_size(0)
    , m_offset(0)
    , m_format("tlv")
    , m_fallback_algorithm("")
{
}

size_t
idprom_t::tlv::code_map(const std::string &code)
{
    auto it = name_to_tag_map.find(code);
    if (it == name_to_tag_map.end()) {
        return 0;
    }
    return it->second;
}

size_t
idprom_t::code_map(const std::string &code)
{
    return tlv::code_map(code);
}

bool
idprom_t::fallback_present() const
{
    if ((m_fallback_algorithm == "w1") && !m_fallback_presence.empty()) {
        if (!m_fallback_status.empty()) {
            // Force a w1 bus reset when we read fallback presence
            sysfs(m_fallback_status).set_value("0");
        }
        return sysfs(m_fallback_presence).get_bool(true);
    }
    return is_present();
}

void
idprom_t::add_computed_fields()
{
    for (const auto &f_it : m_computed) {
        const std::string &name(f_it.first);
        const cfield_t &field(f_it.second);

        const auto &tv_it = m_decoded.find(field.m_from);
        if (tv_it == m_decoded.end()) {
            m_decoded[name].push_back(field.m_default);
            continue;
        }
        std::string key(tv_it->second[0]);
        if (!field.m_bits.empty()) {
            unsigned long long v = 0;
            unsigned long long s = std::stoull(key, nullptr, 0);
            for (const auto &bit : field.m_bits) {
                 v = (v << 1ull) | ((s >> bit) & 1);
            }
            key = std::to_string(v);
        }

        const auto &new_v_it = field.m_values.find(key);
        if (new_v_it == field.m_values.end()) {
            m_decoded[name].push_back(key);
        } else {
            m_decoded[name].push_back(new_v_it->second);
        }
    }

    // Hardcoded rules

    //
    //  Rule 1: Manufacturing date and location are decoded
    //          from serial number.
    //
    decode_serial_no("CHASSIS_SERIAL", "CHASSIS_");
    decode_serial_no("PCB_SERIAL", "PCB_");
}

const fs::path
idprom_t::path(bool resolved) const
{
    if (!resolved || fs::exists(m_path)) {
        return m_path;
    }
    std::string s(m_path.string());
    if (s.find("[") == std::string::npos) {
        return m_path;
    }

    std::string r(name());
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);

    std::regex rx("[^A-Z0-9_]+");
    std::string r_normalized = std::regex_replace(r, rx, "_");
    return "/run/devmap/eeproms/" + r_normalized;
}

void
idprom_t::decode_serial_no(const std::string &tag,
                           const std::string &prefix)
{
    const auto &it = m_decoded.find(tag);
    if (it != m_decoded.end()) {
        const auto &serial(it->second[0]);
        m_decoded[prefix + "MFG_LOCATION"].push_back(serial.substr(0, 3));
        try {
            //          LLLXXWW.* where:
            //             LLL is a location code,
            //             XX is the manufacture year + 4
            //             WW is the week number
            iso_week::year y(std::stoi(serial.substr(3, 2)) - 4 + 2000);
            iso_week::weeknum w(std::stoi(serial.substr(5, 2)));

            iso_week::year_weeknum_weekday ymd(y, w, iso_week::literals::mon);
            std::stringstream s;
            s << date::year_month_day(ymd);
            m_decoded[prefix + "MFG_DATE"].push_back(s.str());
        } catch (...) {
        }
    }
}

bool
idprom_t::is_present() const
{
    return object_t::is_present() && fs::exists(path(true));
}

bool
idprom_t::matches_id(const std::string &given) const
{
    if (object_t::matches_id(given)) {
        return true;
    }
    return given == path(false).string() ||
           given == path(true).string();
}

void
to_json(json& j, const idprom_t::cfield_t &obj)
{
    j = json{
             {"from", obj.m_from},
             {"bits", obj.m_bits},
             {"default", obj.m_default},
             {"values", obj.m_values}
            };
}

void
from_json(const json& j, idprom_t::cfield_t &obj)
{
    obj.m_from = j.value("from", "");
    obj.m_default = j.value("default", "");
    if (j.contains("bits")) {
        j.at("bits").get_to(obj.m_bits);
    } else {
        obj.m_bits.clear();
    }
    if (j.contains("values")) {
        j.at("values").get_to(obj.m_values);
    } else {
        obj.m_values.clear();
    }
}

void
to_json(json& j, const idprom_t& obj)
{
    const object_t& base = obj;

    j = json{
             {"object", base},
             {"size", obj.m_size},
             {"offset", obj.m_offset},
             {"format", obj.m_format},
             {"path", obj.m_path},
             {"fallback_algorithm", obj.m_fallback_algorithm},
             {"fallback_status", obj.m_fallback_status},
             {"fallback_presence", obj.m_fallback_presence},
             {"fallback", obj.m_fallback},
             {"computed_fields", obj.m_computed}
            };
}

void
from_json(const json& j, idprom_t& obj)
{
    object_t &base = obj;

    from_json(j, base);
    obj.m_size = j.value("size", 0);
    obj.m_offset = j.value("offset", 0);
    obj.m_format = j.value("format", "tlv");
    obj.m_path = j.value("path", "");
    obj.m_fallback_algorithm = j.value("fallback_algorithm", "");
    obj.m_fallback_status = j.value("fallback_status", "");
    obj.m_fallback_presence = j.value("fallback_presence", "");
    if (j.contains("fallback")) {
        j.at("fallback").get_to(obj.m_fallback);
    } else {
        obj.m_fallback.clear();
    }
    if (j.contains("computed_fields")) {
        j.at("computed_fields").get_to(obj.m_computed);
    } else {
        obj.m_computed.clear();
    }
}

}; // namespace bsp2
