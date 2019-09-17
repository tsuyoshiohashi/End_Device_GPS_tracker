/*
*   appdata.c
*/
#include <stdio.h>
#include <gps.h>
#include <string.h>

#include "cayenne_lpp.h"

#include "appdata.h"

int set_app_data(  uint8_t* telegram ){

    //// GPS version ////
    loc_t gps_data;
    gps_location(&gps_data);

    //// Cayenne_lpp format ////
    cayenne_lpp_t lpp;
    uint8_t channel=1;      // set application channel
    
    cayenne_lpp_reset( &lpp); 
    cayenne_lpp_add_gps( &lpp, channel, gps_data.latitude, gps_data.longitude, gps_data.altitude);

    memcpy(telegram, lpp.buffer, lpp.cursor);
    printf("\tApp Data:");      
    for( int i=0;i< lpp.cursor; i++){
        printf("%#02x:",lpp.buffer[i]);
    }
    printf("\n");      
    printf("\tLatitude,%3.4lf,Longitude,%3.4lf,Altitude,%3.1lf\n", gps_data.latitude, gps_data.longitude,gps_data.altitude);
    //printf("\tSpeed,%3.4lf,Course,%3.4lf\n", gps_data.speed, gps_data.course);
    
    return(lpp.cursor);
}
// end of appdata.c