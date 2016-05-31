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
        time_out_in_sec = 2;
        endian_representation = 0;
    };

    static std::string getId() {
        return std::string("advanced_configuration");
    };

    short time_out_in_sec;
    CORBA::Long endian_representation;
};

inline bool operator>>= (const CORBA::Any& a, advanced_configuration_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("advanced_configuration::time_out_in_sec")) {
        if (!(props["advanced_configuration::time_out_in_sec"] >>= s.time_out_in_sec)) return false;
    }
    if (props.contains("advanced_configuration::endian_representation")) {
        if (!(props["advanced_configuration::endian_representation"] >>= s.endian_representation)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const advanced_configuration_struct& s) {
    redhawk::PropertyMap props;
 
    props["advanced_configuration::time_out_in_sec"] = s.time_out_in_sec;
 
    props["advanced_configuration::endian_representation"] = s.endian_representation;
    a <<= props;
}

inline bool operator== (const advanced_configuration_struct& s1, const advanced_configuration_struct& s2) {
    if (s1.time_out_in_sec!=s2.time_out_in_sec)
        return false;
    if (s1.endian_representation!=s2.endian_representation)
        return false;
    return true;
}

inline bool operator!= (const advanced_configuration_struct& s1, const advanced_configuration_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
