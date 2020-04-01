#include "ds18b20.h"

#include <unistd.h>
#include <string>
#include <iostream>


int main()
{

    using namespace std;

    string ds18b20_device = find_ds18b20_device();
    cout<<"ds18b20_device: '"<<ds18b20_device<<"'"<<endl;
    if(ds18b20_device == "")
        return 1;

    int retries = 5;
    while(retries--)
    {
        cout<<"temp = "<<read_temp_from_ds18b20(ds18b20_device)<<endl;
        sleep(1);
    }

    return 0;
}
