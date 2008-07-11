//This source code file is written by yellowstar.

//**********ENDIANS*******************************
#define ENDIAN_LITTE 0
#define ENDIAN_BIG 1
bool machine_endian=ENDIAN_LITTE;

void CheckEndianA(void* input, int input_length)
{
	 machine_endian=ENDIAN_LITTE;
}

void ConvertEndian(void* input, void* output, int input_length)
{
     unsigned char *in, *out;
     in=(unsigned char*)malloc((size_t)input_length);
     out=(unsigned char*)malloc((size_t)input_length);
     memset(out,0,(size_t)input_length);
     memset(in,0,(size_t)input_length);
     memcpy(in,input,(size_t)input_length);

     int I=input_length;
     int i;
		for(i=1; i<=input_length; i++)
		{
			out[I-1]=in[i-1];
             I--;
		}

     memcpy(output,out,(size_t)input_length);
}

//**********************AVS ENDIAN*******************************************************
void ConvertAVSEndian(AVS_header *avs)
{
     ConvertEndian(&avs->header_length,&avs->header_length,4);
     ConvertEndian(&avs->MAC_timestamp,&avs->MAC_timestamp,8);
     ConvertEndian(&avs->host_timestamp,&avs->host_timestamp,8);
     ConvertEndian(&avs->PHY_type,&avs->PHY_type,4);
     ConvertEndian(&avs->channel,&avs->channel,4);
     ConvertEndian(&avs->data_rate,&avs->data_rate,4);
     ConvertEndian(&avs->antenna,&avs->antenna,4);
     ConvertEndian(&avs->priority,&avs->priority,4);
     ConvertEndian(&avs->SSI_type,&avs->SSI_type,4);
     ConvertEndian(&avs->RSSI_signal,&avs->RSSI_signal,4);
     ConvertEndian(&avs->preamble,&avs->preamble,4);
     ConvertEndian(&avs->encoding_type,&avs->encoding_type,4);
}
