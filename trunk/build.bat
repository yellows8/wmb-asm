PATH =  PATH + "C:\AndrewConsoleStuff\wmb_asm\wmb_asm\wmb_asm"

"C:\Documents and Settings\Andrew\My Documents\Projects\filecmp\Output\MingW\filecmp.exe" "C:\AndrewConsoleStuff\wmb_asm\wmb_asm\binaries\HULKSMASHDS_NTRJ_8P__500.nds" "C:\Downloads\HULKSMASHDS_NTRJ8P_596EC69F(2)\HULKSMASHDS_NTRJ8P_596EC69F.nds" "C:\AndrewConsoleStuff\wmb_asm\wmb_asm\filecmp_log.txt"

#--wmb_asm.exe -rsa -nds_dirbinaries more_captures
--wmb_asm.exe -nds_dirbinaries captures
#--wmb_asm.exe captures\captureMulti2.cap
--"C:\AndrewConsoleStuff\wmb_asm\wmb_asm\wmb_asm.exe" captures\captureBBA.cap
#--"C:\Documents and Settings\Andrew\My Documents\Projects\filecmp\Output\MingW\filecmp.exe" "captures\NINTENDO    .nds" "C:\Downloads\client\NINTENDO    _NTRJ01_5F39.nds" filecmp_log.txt
--wmb_asm.exe -nds_dirbinaries captures\capturePicross.cap captures\captureNamco.cap captures\captureRobo.cap captures\captureSudoku2.cap captures\captureSalad.cap captures\captureTash.cap captures\captureCard.cap
--wmb_asm.exe -nds_dirbinaries captures
--wmb_asm.exe -nds_dirbinaries -rsa more_captures
--wmb_asm.exe -rsa -run -nostop -nds_dirbinaries captures/captureCard.cap
--ndsrsa.exe verify nintendo "binaries/KURUKESHI!_NTRJ_01__962.nds"

--C:\Dev-Cpp\bin\gprof.exe C:\AndrewConsoleStuff\wmb_asm\wmb_asm\gmon.out
pause
