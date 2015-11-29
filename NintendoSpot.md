This page will describe the Nintendo Spot, and the Nintendo Zone Internet communications. The Nintendo Spot series is the successor to DS Download Stations in Japan and Great Britain. [Nintendo Japanese web page](http://www.nintendo.co.jp/ds/ds_station/howto/index.html) about Nintendo Spot series. [GB web page.](http://ms.nintendo-europe.com/dscentre/download_service.html) This service downloads demos and other content from Internet. The names of each revision is as follows: Nintendo Spot, DS Station, and Nintendo Zone. DS Station has no connection to [Touch!Try!DS!](DSStation.md), referred to on this wiki as "DS Station". On this wiki, the DS Station revision may be referred to by wireless "DS Station".
As of February 2, 2010, this service has the exact same demos that the Japanese Nintendo Channel has, although different servers are used for this service and Nintendo Channel.(Except for the Flipnote Viewer demo on JP NinCh.) In the past new demos were exclusive to this service for a while, then added to Nintendo Channel. Now demos are added to this service and Nintendo Channel at the same time.

# Security #

Every game store in Japan/UK has an access point for intended only NDS. These access points use WEP. The WEP key is generated from the SSID. No configuration is needed for the pzA AP, for Nintendo software. The WEP key seems to be generated from a heavily modified SHA-1 hash of the whole authentication parameter generation output data. This has been implemented. The code for generating the WEP key will not be released. Since the authentication parameter is unique per SSID, the WEP key is unique per SSID. (However with the WEP key dumped from NinSpotDumpPatch, or even the key obtained through WEP cracking, any WiFi device should be able to connect to the AP.)
Server communications are secured with https and SSL/TLS. In addition, an authentication mechanism is used. The input wifi struct is the 32 byte SSID. The authentication mechanism seems to Base64 decode the wifi struct, XOR that output with a key, move all bytes around, and lastly, XOR again with another key. These keys are text keys. Once this is done, the first 8 bytes must match "NDWCSHAP". The 9 bytes following 1 byte after this is used for the authentication parameter. The byte following "NDWCSHAP" must be non-zero, otherwise the 9 bytes are not copied to the authentication parameter buffer. But, if the "NDWCSHAP" string is not found,(no null-terminator) the Nin Spot series client will use a null authentication parameter; which never works.(Server responds with an error meaning invalid authentication parameter.) NTR SDK contains code for running this mechanism,(The code used for the authentication mechanism by Nin Spot series clients is NTR SDK code) to validate if the "NDWCSHAP" string exists in the final output. This is likely how pzA APs are detected.(The extra tag is checked for as well.) This authentication mechanism also seems to double as the mechanism for store-exclusive content. Stores using this, would have an additional option, below the initial screen displaying the "DS Download Service" option. Stores using this usually display info about the store, ect. The demos distributed via this server are RSA-1024 signed. The "combined hash", exponent, and public key from WMB are used for downloaded Nintendo Spot series server binaries verification.

The Base64 decoder uses the standard Base64 alphabet. The translation function is similar to the standard translation mechanism, except if blocks are used rather than a table, and the following is added to translation: if the input char is '=', then Base64 alphabet index 0 is used.('A') And if input char is not '=', and all other if statements fail, the result is s32 -1. The output is not anded with 0x3f, thus overflow with -1 occurs. On the server, it probably checks the authentication parameter against a database of parameters for each AP. If no entry is found, the error for invalid authentication parameter is returned.

The Base64 translation function is implemented as follows. The offset buffer and the XOR keys are defined as well, along with the code for the offset buffer.
```
unsigned char Offset_Array[24] = {0x17, 0x14, 0x11, 0x0D, 0x0B, 0x06, 0x0F, 0x0E, 0x09, 0x15, 0x0C, 0x04, 0x02, 0x01, 0x12, 0x10, 0x05, 0x03, 0x13, 0x0A, 0x07, 0x08, 0x00, 0x16};
char XORKey1[24] = "952uybjnpmu903bia@bk5m[-";
char XORKey2[24] = "38g6zxjk20gvmv]6^=j&%vY1";

int GenerateBase64Byte(unsigned char byte)
{
    if(byte>0x40 && byte<0x5b)return byte - 0x41;
    if(byte>0x60 && byte<0x7b)return (byte - 0x61) + 0x1a;//Or if(...)return byte - 0x47;
    if(byte>0x2f && byte<0x3a)return (byte - 0x30) + 0x34;//Or if(...)return byte + 0x4;
    if(byte==0x2b)return 0x3e;
    if(byte==0x2f)return 0x3f;
    if(byte!=0x3d)return -1;
    if(byte==0x3d)return 0;

    return 0;
}

//Code for the byte moving stage:
for(api = 0; api<24; api++)
{
    api &= 0xff;
    if(offset_array[api]==0xFF)continue;

    tempoffset = api;
    tempval2 = apnum[api];

        do
        {
            tempval = tempoffset;
            tempoffset = offset_array[tempoffset];
            tempval3 = offset_array[api];
            tempval4 = apnum[tempoffset];

            api = tempoffset;
            apnum[tempval3] = tempval2;

            offset_array[tempval] = 0xFF;
            tempval2 = tempval4;

        } while(offset_array[tempoffset]!=0xFF);
}
```

DS Station AP adds the following tag to all frames, except data, ect. However, this tag is not required for the client to connect: all you need is a proper SSID and WEP key for your router/AP, and the client connects. However, the client sends the AP MAC in https connections to the server.(The SSID and WEP key needs to be from a real "DS Station" AP in either JP or GB.) ID: 0xdd Vendor: 0x00 0x16 0x18 Data: 0x02 0x00 0x13 An AP must have this tag and a proper pzA SSID in order for the Nintendo Spot series clients to connect.(DSi won't connect with the "DS Station" tag in the beacons. The only way to find the correct tag is to have someone goto a Nintendo Zone location and run packet capture.)

## Security measures ##

There are many security measures to slow down breaking on the service, most of which are in route 2. Route 1 was attempted first, but sadly the fact that the implementation of the data moving stage of the authentication parameter generation was broken, wasn't discovered until the service was broken with route 2.

### Route 1 ###
  * 1) Find the the function generating the authentication parameter by RE.
  * 2) RE the code generating the authentication parameter.
  * 3) Implement the code.
  * 4) After a lot of debugging, successfully generate the authentication parameter.

### Route 2 ###
  * 1) Modify Linux hostapd to send the extra tags so the client recognizes the AP as a "real" pzA AP.
  * 2) Fix NinSpotDumpPatch bugs and dump the WEP key. Set hostapd to use the dumped WEP key.
  * 3) Tried dumping the authentication parameter with NinSpotDumpPatch, was stumped by a dumping bug.
  * 4) Tried patching the client to use http rather than https, to expose the authentication parameter. The URLs were changed to another server, which redirects to the official server.
  * 5) The client is rigged to not send any parameters needed to connect to the server, including the authentication parameters.(Core parameters sent for all https requests.)
  * 6) Number 5 led to dumping the authentication parameter, by slowly dumping it with a hack for the client.(Which took 1 minute per load due to dynamic patching. The bug that caused resorting to dumping the param slowly was fixed after the service was broken.)
  * 7) Authentication parameter dumped, can now access the server.

# Nintendo Zone #

Nintendo Zone is the latest revision of the Nintendo Spot revision. Nintendo Zone is compatible with both the DSi and the older models. In the future, once server communication information is gathered about Nintendo Zone, Nintendo Zone communications information will be added here. However, Nintendo Zone comms are likely identical to "DS Station", as they probably use similar servers. Official NOA information about Nintendo Zone: [Screenshots/instructions](http://www.nintendo.com/bin/w3I-XYyMEgk1VUUqyo5k-P4eQc_mlXDU/mcHH5cHLGbg5AJQIa_x2nLkBLEUlFmEJ.pdf)
On January 8, 2010, the DSi Nintendo Zone client  software for all regions,(the "built-in Viewer") was updated. Although version was changed from 2 to 3, connecting to the DS Station server still works, although still with invalid authentication parameter errors. In general, Nintendo Zone replaced Nintendo Spot in Japan, while wireless "DS Station" is still used in game stores. Wireless "DS Station" used by game stores, don't really use the per store unique content, while Nintendo Zone available in McDonalds and other [companies](http://www.nintendo.co.jp/ds/nintendozone/index.html) uses this unique per AP content functionality.

# Exploits #

Some info on an DS Station exploit is available on [dsibrew.](http://dsibrew.org/wiki/Nintendo_Zone#Exploits) This exploit could be adapted to Nintendo Zone, if an Nintendo Zone AP capture and the Nintendo Zone binary is obtained. It is unknown if this exploit works with Nintendo Zone and DSi. A volunteer to capture Nintendo Zone AP traffic and the WMB binary was found a long while ago, but it is unknown when he'll capture. A means of injecting a HTTP reply is needed, to use the exploit without modifying the client.(DNS redirection and a https proxy/forwarder app can be used as well.) The exploit is only available on SVN. SVN web interface is available [here](http://code.google.com/p/wmb-asm/source/browse/#svn/trunk/nzoneurlstacksmash), SVN URL available [here.](http://wmb-asm.googlecode.com/svn/trunk/nzoneurlstacksmash) If you want to use the exploit at home, you also need a HTTPS forwarder/proxy, like httpsforwarder available in SVN.