This page attempts to explain how to obtain captures, along with external sources. Also, link(s) to capture download(s) are available.

## Obtaining captures ##

See this [page](http://wiki.akkit.org/DSDemoCapture) at the bottom, and also this second [page](http://www.bottledlight.com/ds/index.php/Wifi/WMBProtocol) near the bottom. For more capturing info, see the Wireshark wiki page: http://wiki.wireshark.org/CaptureSetup/WLAN

You'll need a wireless card capable of raw packet capture. Capture on Windows can be done
with Ralink cards on this [list](http://ralink.rapla.net/), and Firefly's capture [tool](http://users.belgacom.net/bn967347/download.htm). On Linux, capture can be done on any card capable of being used in monitor mode, and usable on Linux.(Network manager GUI software needs wireless disabled in order to use monitor mode.) airodump-ng captures can only be used with SVN wmb\_asm. However, not all Linux wireless drivers permit capture of management frames. This is required for wmb\_asm currently, even with latest SVN.

## Captures available for download ##

### [WMB](WMB.md) ###

American DS Download Station Vol.1 Client [capture](http://wmb-asm.googlecode.com/files/american_dsdlstationvol1_clientcap.zip) captured by Cboomf. This capture is placed under WMB captures because only the client is captured, no demos were captured.

### [DS Download Station](DSDownloadStation.md) ###
No DS Download Station captures are available for download.