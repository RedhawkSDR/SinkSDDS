#ifndef STRUCTPROPS_H
#define STRUCTPROPS_H

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

*******************************************************************************************/

#include <ossie/CorbaUtils.h>
#include <CF/cf.h>
#include <ossie/PropertyMap.h>

struct network_settings_struct {
    network_settings_struct ()
    {
        interface = "eth0";
        ip_address = "127.0.0.1";
        port = 29495;
        vlan = 0;
    };

    static std::string getId() {
        return std::string("network_settings");
    };

    std::string interface;
    std::string ip_address;
    CORBA::Long port;
    unsigned short vlan;
};

inline bool operator>>= (const CORBA::Any& a, network_settings_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("network_settings::interface")) {
        if (!(props["network_settings::interface"] >>= s.interface)) return false;
    }
    if (props.contains("network_settings::ip_address")) {
        if (!(props["network_settings::ip_address"] >>= s.ip_address)) return false;
    }
    if (props.contains("network_settings::port")) {
        if (!(props["network_settings::port"] >>= s.port)) return false;
    }
    if (props.contains("network_settings::vlan")) {
        if (!(props["network_settings::vlan"] >>= s.vlan)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const network_settings_struct& s) {
    redhawk::PropertyMap props;
 
    props["network_settings::interface"] = s.interface;
 
    props["network_settings::ip_address"] = s.ip_address;
 
    props["network_settings::port"] = s.port;
 
    props["network_settings::vlan"] = s.vlan;
    a <<= props;
}

inline bool operator== (const network_settings_struct& s1, const network_settings_struct& s2) {
    if (s1.interface!=s2.interface)
        return false;
    if (s1.ip_address!=s2.ip_address)
        return false;
    if (s1.port!=s2.port)
        return false;
    if (s1.vlan!=s2.vlan)
        return false;
    return true;
}

inline bool operator!= (const network_settings_struct& s1, const network_settings_struct& s2) {
    return !(s1==s2);
}

struct sdds_settings_struct {
    sdds_settings_struct ()
    {
        standard_format = 1;
        original_format = 0;
        spectral_sense = 0;
        complexity = 0;
        frequency = 154000000;
        frequency_change_rate = 0;
        time_tag_valid = true;
        time_tag_estimation = true;
        user_id = "anonymous";
    };

    static std::string getId() {
        return std::string("sdds_settings");
    };

    short standard_format;
    short original_format;
    short spectral_sense;
    short complexity;
    double frequency;
    double frequency_change_rate;
    bool time_tag_valid;
    bool time_tag_estimation;
    std::string user_id;
};

inline bool operator>>= (const CORBA::Any& a, sdds_settings_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("sdds_settings::standard_format")) {
        if (!(props["sdds_settings::standard_format"] >>= s.standard_format)) return false;
    }
    if (props.contains("sdds_settings::original_format")) {
        if (!(props["sdds_settings::original_format"] >>= s.original_format)) return false;
    }
    if (props.contains("sdds_settings::spectral_sense")) {
        if (!(props["sdds_settings::spectral_sense"] >>= s.spectral_sense)) return false;
    }
    if (props.contains("sdds_settings::complexity")) {
        if (!(props["sdds_settings::complexity"] >>= s.complexity)) return false;
    }
    if (props.contains("sdds_settings::frequency")) {
        if (!(props["sdds_settings::frequency"] >>= s.frequency)) return false;
    }
    if (props.contains("sdds_settings::frequency_change_rate")) {
        if (!(props["sdds_settings::frequency_change_rate"] >>= s.frequency_change_rate)) return false;
    }
    if (props.contains("sdds_settings::time_tag_valid")) {
        if (!(props["sdds_settings::time_tag_valid"] >>= s.time_tag_valid)) return false;
    }
    if (props.contains("sdds_settings::time_tag_estimation")) {
        if (!(props["sdds_settings::time_tag_estimation"] >>= s.time_tag_estimation)) return false;
    }
    if (props.contains("sdds_settings::user_id")) {
        if (!(props["sdds_settings::user_id"] >>= s.user_id)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sdds_settings_struct& s) {
    redhawk::PropertyMap props;
 
    props["sdds_settings::standard_format"] = s.standard_format;
 
    props["sdds_settings::original_format"] = s.original_format;
 
    props["sdds_settings::spectral_sense"] = s.spectral_sense;
 
    props["sdds_settings::complexity"] = s.complexity;
 
    props["sdds_settings::frequency"] = s.frequency;
 
    props["sdds_settings::frequency_change_rate"] = s.frequency_change_rate;
 
    props["sdds_settings::time_tag_valid"] = s.time_tag_valid;
 
    props["sdds_settings::time_tag_estimation"] = s.time_tag_estimation;
 
    props["sdds_settings::user_id"] = s.user_id;
    a <<= props;
}

inline bool operator== (const sdds_settings_struct& s1, const sdds_settings_struct& s2) {
    if (s1.standard_format!=s2.standard_format)
        return false;
    if (s1.original_format!=s2.original_format)
        return false;
    if (s1.spectral_sense!=s2.spectral_sense)
        return false;
    if (s1.complexity!=s2.complexity)
        return false;
    if (s1.frequency!=s2.frequency)
        return false;
    if (s1.frequency_change_rate!=s2.frequency_change_rate)
        return false;
    if (s1.time_tag_valid!=s2.time_tag_valid)
        return false;
    if (s1.time_tag_estimation!=s2.time_tag_estimation)
        return false;
    if (s1.user_id!=s2.user_id)
        return false;
    return true;
}

inline bool operator!= (const sdds_settings_struct& s1, const sdds_settings_struct& s2) {
    return !(s1==s2);
}

struct advanced_configuration_struct {
    advanced_configuration_struct ()
    {
        buffer_size = 200000;
        time_out_in_sec = 2;
        number_of_packets_in_burst = 100;
        throttle_time_between_packet_bursts = 2;
        endian_representation = 0;
        use_bulkio_sri = false;
    };

    static std::string getId() {
        return std::string("advanced_configuration");
    };

    CORBA::Long buffer_size;
    short time_out_in_sec;
    CORBA::Long number_of_packets_in_burst;
    CORBA::Long throttle_time_between_packet_bursts;
    CORBA::Long endian_representation;
    bool use_bulkio_sri;
};

inline bool operator>>= (const CORBA::Any& a, advanced_configuration_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("advanced_configuration::buffer_size")) {
        if (!(props["advanced_configuration::buffer_size"] >>= s.buffer_size)) return false;
    }
    if (props.contains("advanced_configuration::time_out_in_sec")) {
        if (!(props["advanced_configuration::time_out_in_sec"] >>= s.time_out_in_sec)) return false;
    }
    if (props.contains("advanced_configuration::number_of_packets_in_burst")) {
        if (!(props["advanced_configuration::number_of_packets_in_burst"] >>= s.number_of_packets_in_burst)) return false;
    }
    if (props.contains("advanced_configuration::throttle_time_between_packet_bursts")) {
        if (!(props["advanced_configuration::throttle_time_between_packet_bursts"] >>= s.throttle_time_between_packet_bursts)) return false;
    }
    if (props.contains("advanced_configuration::endian_representation")) {
        if (!(props["advanced_configuration::endian_representation"] >>= s.endian_representation)) return false;
    }
    if (props.contains("advanced_configuration::use_bulkio_sri")) {
        if (!(props["advanced_configuration::use_bulkio_sri"] >>= s.use_bulkio_sri)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const advanced_configuration_struct& s) {
    redhawk::PropertyMap props;
 
    props["advanced_configuration::buffer_size"] = s.buffer_size;
 
    props["advanced_configuration::time_out_in_sec"] = s.time_out_in_sec;
 
    props["advanced_configuration::number_of_packets_in_burst"] = s.number_of_packets_in_burst;
 
    props["advanced_configuration::throttle_time_between_packet_bursts"] = s.throttle_time_between_packet_bursts;
 
    props["advanced_configuration::endian_representation"] = s.endian_representation;
 
    props["advanced_configuration::use_bulkio_sri"] = s.use_bulkio_sri;
    a <<= props;
}

inline bool operator== (const advanced_configuration_struct& s1, const advanced_configuration_struct& s2) {
    if (s1.buffer_size!=s2.buffer_size)
        return false;
    if (s1.time_out_in_sec!=s2.time_out_in_sec)
        return false;
    if (s1.number_of_packets_in_burst!=s2.number_of_packets_in_burst)
        return false;
    if (s1.throttle_time_between_packet_bursts!=s2.throttle_time_between_packet_bursts)
        return false;
    if (s1.endian_representation!=s2.endian_representation)
        return false;
    if (s1.use_bulkio_sri!=s2.use_bulkio_sri)
        return false;
    return true;
}

inline bool operator!= (const advanced_configuration_struct& s1, const advanced_configuration_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
