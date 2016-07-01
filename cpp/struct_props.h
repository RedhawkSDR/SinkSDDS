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
        endian_representation = 1;
    };

    static std::string getId() {
        return std::string("sdds_settings");
    };

    short standard_format;
    short original_format;
    short spectral_sense;
    CORBA::Long endian_representation;
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
    if (props.contains("sdds_settings::endian_representation")) {
        if (!(props["sdds_settings::endian_representation"] >>= s.endian_representation)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sdds_settings_struct& s) {
    redhawk::PropertyMap props;
 
    props["sdds_settings::standard_format"] = s.standard_format;
 
    props["sdds_settings::original_format"] = s.original_format;
 
    props["sdds_settings::spectral_sense"] = s.spectral_sense;
 
    props["sdds_settings::endian_representation"] = s.endian_representation;
    a <<= props;
}

inline bool operator== (const sdds_settings_struct& s1, const sdds_settings_struct& s2) {
    if (s1.standard_format!=s2.standard_format)
        return false;
    if (s1.original_format!=s2.original_format)
        return false;
    if (s1.spectral_sense!=s2.spectral_sense)
        return false;
    if (s1.endian_representation!=s2.endian_representation)
        return false;
    return true;
}

inline bool operator!= (const sdds_settings_struct& s1, const sdds_settings_struct& s2) {
    return !(s1==s2);
}

struct sdds_attach_settings_struct {
    sdds_attach_settings_struct ()
    {
        time_tag_valid = true;
        user_id = "anonymous";
        downstream_give_sri_priority = false;
    };

    static std::string getId() {
        return std::string("sdds_attach_settings");
    };

    bool time_tag_valid;
    std::string user_id;
    bool downstream_give_sri_priority;
};

inline bool operator>>= (const CORBA::Any& a, sdds_attach_settings_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("sdds_attach_settings::time_tag_valid")) {
        if (!(props["sdds_attach_settings::time_tag_valid"] >>= s.time_tag_valid)) return false;
    }
    if (props.contains("sdds_attach_settings::user_id")) {
        if (!(props["sdds_attach_settings::user_id"] >>= s.user_id)) return false;
    }
    if (props.contains("sdds_attach_settings::downstream_should_use_sri")) {
        if (!(props["sdds_attach_settings::downstream_should_use_sri"] >>= s.downstream_give_sri_priority)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sdds_attach_settings_struct& s) {
    redhawk::PropertyMap props;
 
    props["sdds_attach_settings::time_tag_valid"] = s.time_tag_valid;
 
    props["sdds_attach_settings::user_id"] = s.user_id;
 
    props["sdds_attach_settings::downstream_should_use_sri"] = s.downstream_give_sri_priority;
    a <<= props;
}

inline bool operator== (const sdds_attach_settings_struct& s1, const sdds_attach_settings_struct& s2) {
    if (s1.time_tag_valid!=s2.time_tag_valid)
        return false;
    if (s1.user_id!=s2.user_id)
        return false;
    if (s1.downstream_give_sri_priority!=s2.downstream_give_sri_priority)
        return false;
    return true;
}

inline bool operator!= (const sdds_attach_settings_struct& s1, const sdds_attach_settings_struct& s2) {
    return !(s1==s2);
}

struct override_sdds_header_struct {
    override_sdds_header_struct ()
    {
        enabled = false;
        dmode = 0;
        bps = 0;
        cx = 0;
        msv = 0;
        ttv = 0;
        sscv = 0;
        msptr = 0;
        msdel = 0;
        frequency = 0;
        dfdt = 0;
    };

    static std::string getId() {
        return std::string("override_sdds_header");
    };

    bool enabled;
    unsigned short dmode;
    unsigned short bps;
    unsigned short cx;
    unsigned short msv;
    unsigned short ttv;
    unsigned short sscv;
    unsigned short msptr;
    unsigned short msdel;
    double frequency;
    double dfdt;
};

inline bool operator>>= (const CORBA::Any& a, override_sdds_header_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("override_sdds_header::enabled")) {
        if (!(props["override_sdds_header::enabled"] >>= s.enabled)) return false;
    }
    if (props.contains("override_sdds_header::dmode")) {
        if (!(props["override_sdds_header::dmode"] >>= s.dmode)) return false;
    }
    if (props.contains("override_sdds_header::bps")) {
        if (!(props["override_sdds_header::bps"] >>= s.bps)) return false;
    }
    if (props.contains("override_sdds_header::cx")) {
        if (!(props["override_sdds_header::cx"] >>= s.cx)) return false;
    }
    if (props.contains("override_sdds_header::msv")) {
        if (!(props["override_sdds_header::msv"] >>= s.msv)) return false;
    }
    if (props.contains("override_sdds_header::ttv")) {
        if (!(props["override_sdds_header::ttv"] >>= s.ttv)) return false;
    }
    if (props.contains("override_sdds_header::sscv")) {
        if (!(props["override_sdds_header::sscv"] >>= s.sscv)) return false;
    }
    if (props.contains("override_sdds_header::msptr")) {
        if (!(props["override_sdds_header::msptr"] >>= s.msptr)) return false;
    }
    if (props.contains("override_sdds_header::msdel")) {
        if (!(props["override_sdds_header::msdel"] >>= s.msdel)) return false;
    }
    if (props.contains("override_sdds_header::frequency")) {
        if (!(props["override_sdds_header::frequency"] >>= s.frequency)) return false;
    }
    if (props.contains("override_sdds_header::dfdt")) {
        if (!(props["override_sdds_header::dfdt"] >>= s.dfdt)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const override_sdds_header_struct& s) {
    redhawk::PropertyMap props;
 
    props["override_sdds_header::enabled"] = s.enabled;
 
    props["override_sdds_header::dmode"] = s.dmode;
 
    props["override_sdds_header::bps"] = s.bps;
 
    props["override_sdds_header::cx"] = s.cx;
 
    props["override_sdds_header::msv"] = s.msv;
 
    props["override_sdds_header::ttv"] = s.ttv;
 
    props["override_sdds_header::sscv"] = s.sscv;
 
    props["override_sdds_header::msptr"] = s.msptr;
 
    props["override_sdds_header::msdel"] = s.msdel;
 
    props["override_sdds_header::frequency"] = s.frequency;
 
    props["override_sdds_header::dfdt"] = s.dfdt;
    a <<= props;
}

inline bool operator== (const override_sdds_header_struct& s1, const override_sdds_header_struct& s2) {
    if (s1.enabled!=s2.enabled)
        return false;
    if (s1.dmode!=s2.dmode)
        return false;
    if (s1.bps!=s2.bps)
        return false;
    if (s1.cx!=s2.cx)
        return false;
    if (s1.msv!=s2.msv)
        return false;
    if (s1.ttv!=s2.ttv)
        return false;
    if (s1.sscv!=s2.sscv)
        return false;
    if (s1.msptr!=s2.msptr)
        return false;
    if (s1.msdel!=s2.msdel)
        return false;
    if (s1.frequency!=s2.frequency)
        return false;
    if (s1.dfdt!=s2.dfdt)
        return false;
    return true;
}

inline bool operator!= (const override_sdds_header_struct& s1, const override_sdds_header_struct& s2) {
    return !(s1==s2);
}

struct status_struct {
    status_struct ()
    {
        current_stream = "";
        stream_on_deck = "";
    };

    static std::string getId() {
        return std::string("status");
    };

    std::string current_stream;
    std::string stream_on_deck;
};

inline bool operator>>= (const CORBA::Any& a, status_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("status::current_stream")) {
        if (!(props["status::current_stream"] >>= s.current_stream)) return false;
    }
    if (props.contains("status::stream_on_deck")) {
        if (!(props["status::stream_on_deck"] >>= s.stream_on_deck)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const status_struct& s) {
    redhawk::PropertyMap props;
 
    props["status::current_stream"] = s.current_stream;
 
    props["status::stream_on_deck"] = s.stream_on_deck;
    a <<= props;
}

inline bool operator== (const status_struct& s1, const status_struct& s2) {
    if (s1.current_stream!=s2.current_stream)
        return false;
    if (s1.stream_on_deck!=s2.stream_on_deck)
        return false;
    return true;
}

inline bool operator!= (const status_struct& s1, const status_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
