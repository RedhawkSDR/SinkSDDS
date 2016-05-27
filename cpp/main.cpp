#include <iostream>
#include "ossie/ossieSupport.h"

#include "SinkSDDS.h"
int main(int argc, char* argv[])
{
    SinkSDDS_i* SinkSDDS_servant;
    Component::start_component(SinkSDDS_servant, argc, argv);
    return 0;
}

