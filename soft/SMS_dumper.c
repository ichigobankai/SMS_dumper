/*
ICHIGO 2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#endif

#include "hid.h" // HID Lib

// HID Special Command
#define SMS_WAKEUP    		0x11  // WakeUP STM32
#define SMS_READ      		0x12  // Read
#define SMS_WRITE_SRAM  	0x13  // Write
#define SMS_WRITE_FLASH     0x14  // Write Flash
#define SMS_ERASE_FLASH     0x15  // Erase Flash

const char * ext[] = {".sms",".sg", ".sc"};

unsigned int crc32(unsigned int seed, const void* data, int data_size){

    unsigned int crc = ~seed;

    static unsigned int crc32_lut[256] = { 0 };
    if(!crc32_lut[1]){
        const unsigned int polynomial = 0xEDB88320;
        unsigned int i, j;
        for (i = 0; i < 256; i++){
            unsigned int crc = i;
            for (j = 0; j < 8; j++)
                crc = (crc >> 1) ^ ((unsigned int)(-(int)(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    {
    	const unsigned char* data8 = (const unsigned char*)data;
    	int n;
    	for (n = 0; n < data_size; n++)
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ data8[n]];
    }
    return ~crc;
}

unsigned int check_buffer(unsigned long adr, unsigned char * buf, unsigned int length){
	unsigned int c=0;
	unsigned int i=0;
	while(i < length){
		c += buf[(adr +i)];
		i++;
	}
	return c;	
}

int main(){
    // Program Var
   	int connect;
   	int check = 1;
	unsigned int verif = 0; 
    unsigned int choixMenu = 0;
    unsigned int choixMapperType = 0;
    unsigned int choixSousMenu = 0;
    unsigned long int buffer_checksum[0xFF] = {0};

    // HID Command Buffer & Var
    unsigned char HIDcommand[64];
    unsigned char buffer_recv[64];
    unsigned char *buffer_rom = NULL;
    unsigned char *buffer_flash = NULL;
    
    char gameName[64];
    int ext_num = 0;
    unsigned long address = 0;
    unsigned long gameSize = 0;
    unsigned long totalGameSize = 0;
    
    unsigned int slotAdr = 0;
    unsigned int slotSize = 0;
    unsigned int slotRegister = 0;

	#if defined(OS_LINUX) || defined(OS_MACOSX)
		struct timeval ostime;
		long microsec_start = 0;
		long microsec_end = 0;
	#elif defined(OS_WINDOWS)
		clock_t microsec_start = clock();
		clock_t microsec_end = clock();
	#endif	

	time_t rawtime;
	time(&rawtime);
	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);	

	unsigned long int i=0, checksum_rom=0, checksum_flash=0;
	unsigned char page=0;
		
    // Output File
    FILE *myfile = NULL;

    printf("\n*** Sega MASTER SYSTEM,");
    printf("\n*** Mark III and SG1000");
    printf("\n*** USB Dumper - ICHIGO");
	#if defined(OS_WINDOWS)
	    printf("\n*** WINDOWS");
	#elif defined(OS_MACOSX)
	    printf("\n*** MACOS");
	#elif defined(OS_LINUX)
	    printf("\n*** LINUX");
	#endif
    printf(" ver. 0.99");
	printf("\n*** Compiled %04d/%02d/%02d\n", timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday);
    printf("-------------------------\n");
    printf(" Detecting... \n");

	connect = rawhid_open(1, 0x0483, 0x5750, -1, -1);
    if(connect <= 0){
        connect = rawhid_open(1, 0x0483, 0x5750, -1, -1);
        if(connect <= 0){
            printf(" SMS Dumper NOT FOUND!\n Exit\n\n");
            return 0;
        }
    }	
    printf(" SMS Dumper detected");
 
    HIDcommand[0] = SMS_WAKEUP;
    rawhid_send(0, HIDcommand, 64, 30);
    while(check){
        connect = rawhid_recv(0, buffer_recv, 64, 30);
        if(buffer_recv[0]!=0xFF){
 		   	printf("-------------------------\n");
           	printf("\n Error reading\n Exit\n\n");
           	rawhid_close(0);
            return 0;
        }else{
            printf(" and ready!\n");
           	check = 0;
           	buffer_recv[0] = 0;
        }
    }

    printf("\n----------MENU-----------\n");
    printf(" 1. Dump ROM\n");
    printf(" 2. Dump S-RAM\n");
    printf(" 3. Write/Erase S-RAM\n");
    printf(" 4. Write FLASH 39SFxxx (max. 512KB)\n");
    printf(" 5. Exit\n");
    printf(" Enter your choice: ");
    scanf("%d", &choixMenu);
    
    switch(choixMenu){
    	
    case 1:  // DUMP ROM
	    printf("\n-------MAPPER TYPE-------\n");
   		printf("  1. Sega\n");
   		printf("  2. SG-1000/Othello Multivision\n");
   		printf("  3. SC-3000\n");
	    printf("  4. Codemasters\n");
	    printf("  5. Korean 1MB\n");
	    printf("  6. Korean MSX 8kb\n");
	    printf("  7. Korean X-in-1\n");
	    printf("  8. Korean \"Jangun\"\n");
	    printf("  9. SG1000 Taiwan Msx\n");
	    printf(" 10. 4 Pak All Action\n");
	    printf(" 11. Exit\n");
	    printf(" Enter your choice: ");
	    scanf("%d", &choixMapperType);
   	
   		if(choixMapperType<11){
    	printf("-------------------------\n");
        printf(" Size to dump in KB: ");
        scanf("%ld", &gameSize);
        printf(" Rom's name (no space/no ext): ");
        scanf("%610s", gameName);

   		printf("-------------------------\n");
        HIDcommand[0] = SMS_READ;
		totalGameSize = gameSize*1024;
		
        buffer_rom = (unsigned char*)malloc(totalGameSize);

		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_start = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_start = clock();
		#endif	
	
		/*
		
		#define MAPPER_SMS_Korean_MSX_8KB				(12)
		// 8KB bank-switching (4 banks) 2KB/bnk 1 - 0x0002 || addr == 0x0003 || addr == 0x0004)
		
		#define MAPPER_SMS_Korean_Janggun				(13)
		// 8KB bank-switching (4 banks) mixed with 16KB bank-switching
		
		#define MAPPER_SMS_4PakAllAction				(14)
		
		#define MAPPER_SG1000_Taiwan_MSX_Adapter_TypeA	(15)
		// 8KB RAM from 0x2000->0x3FFF + regular 2KB ram in 0xC000-0xFFFF range
		
		#define MAPPER_SMS_Korean_Xin1					(16)
		// Mapping register at 0xFFFF to map 32 KB at 0x0000->0x8000 //easy
		
		*/
		
        ext_num = 0; //default ext .sms 
  		slotSize = 16384; //default slotsize
        HIDcommand[7] = 0;
 		
       	switch(choixMapperType){
        	case 1: //sega
        		HIDcommand[7] = 1; //SEGA mapper add M0-7 as A15/OE
	       		break;
        	case 2: //sg1000
        		ext_num = 1; //.sg
        		HIDcommand[7] = 1;
	       		break;
        	case 3: //sc3000
        		ext_num = 2; //.sc
        		HIDcommand[7] = 1;
	       		break;	       		
        	case 4: //codemasters
        		HIDcommand[7] = 2; //use Mreq and Clock
	      		break;
        	case 5: //korean "jang pung 3"
       			HIDcommand[7] = 0;
	       		break;				
        	case 6: //korean max8kb
        		HIDcommand[7] = 0;
				slotSize = 2048;
	       		break;	
        	case 7: //korean x-in-1
        		HIDcommand[7] = 4;
	       		break;	
        	case 8: //korean "janggun"
        		HIDcommand[7] = 0;
	       		break;	
        	case 9: //SG1000 taiwan
        		HIDcommand[7] = 0;
	       		break;	
        	case 10: //4 Pak
        		HIDcommand[7] = 3;  //use Mreq - M0-7
	       		break;
        }
	       
        while(address < totalGameSize){

      		HIDcommand[3] = 0; //default no mapper
	    	HIDcommand[4] = (address/slotSize); //page MAPPER		    	
 
        	if(address<0x4000){
        		slotAdr = address;
        		switch(choixMapperType){
	        		case 1: slotRegister = 0xFFFD;	HIDcommand[3] = 1; break;
	        		case 4: slotRegister = 0;		HIDcommand[3] = 1; break;
	        		case 6: 
			        		if(address<0x800){ slotRegister = 0;  HIDcommand[3] = 1;}
			        		else if(address<0x1000){ slotRegister = 1; HIDcommand[3] = 1;}
			        		else if(address<0x1800){ slotRegister = 2; HIDcommand[3] = 1;}
			        		else{ slotRegister = 3; HIDcommand[3] = 1;}
			        		break;
	        		case 10: slotRegister = 0x3FFE; HIDcommand[3] = 1; break;
	        		default: slotRegister = 0;		HIDcommand[3] = 0;
        		}
        		
        	}else if(address<0x8000){
        		slotAdr = address;
        		switch(choixMapperType){
	        		case 1: slotRegister = 0xFFFE;	HIDcommand[3] = 1; break;
	        		case 4: slotRegister = 0x4000;	HIDcommand[3] = 1; break;
	        		case 6: slotRegister = 3; 		HIDcommand[3] = 1; break;
	        		case 10: slotRegister = 0x7FFE;	HIDcommand[3] = 1; break;
	        		default: slotRegister = 0;		HIDcommand[3] = 0;
        		}
	    		    	       		
        	}else if(address>0x7FFF){
        		switch(choixMapperType){
	        		case 1: //sega
	        		case 7: //x-in1 map 0-0x7FFF
	        		case 8: //janggun
	        				slotRegister = 0xFFFF;	HIDcommand[3] = 1; break;
	        		case 4: slotRegister = 0x8000;	HIDcommand[3] = 1; break;
	        		case 5: slotRegister = 0xA000;	HIDcommand[3] = 1; break;
	        		case 6: slotRegister = 3; 		HIDcommand[3] = 1; break;
	        		case 10: slotRegister = 0xBFFE;	HIDcommand[3] = 1; break;
	        		default: slotRegister = 0;		HIDcommand[3] = 0;
        		}
        		
	        	if(HIDcommand[7]!=4){
	        		slotAdr = 0x8000 + (address & 0x3FFF);     	
	        		}else{
	        		slotAdr = 0x8000 + (address & 0x7FFF);     	        		
	        	}
        	}
		        
            HIDcommand[1] = slotAdr & 0xFF;				//lo adr
            HIDcommand[2] = (slotAdr & 0xFF00)>>8; 		//hi adr	    
		    HIDcommand[5] = (slotRegister & 0xFF); 		//reg MAPPER lo adr
		    HIDcommand[6] = (slotRegister & 0xFF00)>>8; //reg MAPPER hi adr

            rawhid_send(0, HIDcommand, 64, 30);
            rawhid_recv(0, (buffer_rom +address), 64, 30);

            address += 64 ;

            printf("\r ROM dump in progress: %lu%% reg:%04X page:%02X/%02lX size:%dKB", ((100 * address)/totalGameSize), slotRegister, HIDcommand[4], (totalGameSize/slotSize)-1, slotSize);
            fflush(stdout);

        }
		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_end = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_end = clock();
		#endif
		
   		printf("\n-------------------------\n");
        printf(" ROM dump complete! \n");
               
        sprintf(gameName, "%s_(%X)%s", gameName, crc32(0, buffer_rom, totalGameSize), ext[ext_num]);
        
		myfile = fopen(gameName,"wb");
	    fwrite(buffer_rom, 1, totalGameSize, myfile);
	    fclose(myfile);
   		
   		}else{
	        printf("\n");
   			free(buffer_rom);
   			rawhid_close(0);
			return 0; //exit	
   		}
        break;	
    
    
    case 2: //DUMP S/F-RAM
    	printf("\n-------DUMP S-RAM--------\n");
       	printf(" Sram's name (no space/no ext): ");
        scanf("%60s", gameName);
   		printf("-------------------------\n");

        HIDcommand[0] = SMS_READ;
		totalGameSize = 8192; //set as 8ko
        
        buffer_rom = (unsigned char*)malloc(totalGameSize);   
        strcat(gameName, "_sram.bin"); 
 		slotSize = 8192;
 		slotRegister = 0xFFFC; 	// slot 2 sega

		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_start = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_start = clock();
		#endif	
		
        while(address < totalGameSize){

		    slotAdr = 0x8000 + (address & 0x3FFF);     	
       		HIDcommand[3] = 1; 								//enable MAPPER
		    HIDcommand[4] = 8;					 			//bit3 = RAM enable ($8000-$bfff)
		    HIDcommand[5] = (slotRegister & 0xFF); 			//reg MAPPER lo adr
		    HIDcommand[6] = (slotRegister & 0xFF00)>>8; 	//reg MAPPER hi adr
 	        
            HIDcommand[1] = slotAdr & 0xFF;					//lo adr
            HIDcommand[2] = (slotAdr & 0xFF00)>>8; 			//hi adr
                    
            rawhid_send(0, HIDcommand, 64, 30);
            rawhid_recv(0, (buffer_rom +address), 64, 30);

            address += 64 ;

            printf("\r SRAM dump in progress: %lu%%", ((100 * address)/totalGameSize));
            fflush(stdout);
        }
        
		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_end = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_end = clock();
		#endif	
		
   		printf("\n-------------------------\n");
        printf(" SRAM dump complete! \n");
		myfile = fopen(gameName,"wb");
	    fwrite(buffer_rom, 1, totalGameSize, myfile);
	    fclose(myfile);
    	break;
 
	case 3: //WRITE S/F-RAM
        HIDcommand[0] = SMS_WRITE_SRAM;
   		totalGameSize = 8192;
        slotRegister = 0xFFFC;  
        buffer_rom = (unsigned char*)malloc(totalGameSize);
		
     	printf("\n-------WRITE S-RAM--------\n");
   		printf(" 1. Write file\n");
	    printf(" 2. Erase\n");
	    printf(" 3. Exit\n");
	    printf(" Enter your choice: ");
	    scanf("%d", &choixSousMenu);

       	if(choixSousMenu == 1){
	       	printf(" SRAM file : ");
	        scanf("%60s", gameName);
			myfile = fopen(gameName,"rb");
		    if(myfile == NULL){
		    	printf(" FLASH file %s not found !\n Exit\n\n", gameName);
   				free(buffer_rom);
   				rawhid_close(0);
		        return 0;
		    }
		    fread(buffer_rom, 1, totalGameSize, myfile);
			fclose(myfile);
       	}else if(choixSousMenu == 2){
			for(unsigned int i=0; i<totalGameSize; i++){
				buffer_rom[i] = 0xFF;
			}
      	   	printf(" Erase SRAM\n");
        }else{
        printf("\n");
		free(buffer_rom);
   		rawhid_close(0);
		return 0;        	
        }
        printf("-------------------------\n");


		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_start = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_start = clock();
		#endif	      
   		//0 to 0x4000 - 256 packets 32bytes to send
        while(address < totalGameSize){
        	
		    slotAdr = 0x8000 +address;     	
            HIDcommand[1] = slotAdr & 0xFF;					//lo adr
            HIDcommand[2] = (slotAdr & 0xFF00)>>8; 			//hi adr
 		    HIDcommand[4] = 8;					 			//bit3 = RAM enable ($8000-$bfff)
		    HIDcommand[5] = (slotRegister & 0xFF); 			//reg MAPPER lo adr
		    HIDcommand[6] = (slotRegister & 0xFF00)>>8; 	//reg MAPPER hi adr
			memcpy((unsigned char *)HIDcommand +7, (unsigned char *)buffer_rom +address, 32);
             
            rawhid_send(0, HIDcommand, 64, 30);
            rawhid_recv(0, buffer_recv, 64, 30); //wait to continue
            
            address += 32;

            printf("\r SRAM write in progress: %lu%%", ((100 * address)/totalGameSize));
            fflush(stdout);
        }
        
		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_end = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_end = clock();
		#endif	
		
   		printf("\n-------------------------\n");
        printf(" SRAM write complete! \n");
    	break;

	case 4: //WRITE FLASH
		
        printf("\n-------WRITE FLASH-------\n");
	    printf(" FILE to write: ");
	    scanf("%60s", gameName);
		myfile = fopen(gameName,"rb");
	    if(myfile == NULL){
	    	printf(" FLASH file %s not found !\n Exit\n\n", gameName);
   			rawhid_close(0);
	        return 0;
	    }
	    fseek(myfile, 0, SEEK_END);
	    totalGameSize = ftell(myfile);
	    rewind(myfile);
	    
	    if(totalGameSize>524288){
	    	printf(" FLASH file too big!\n Max 524288 bytes (file : %lu)\nExit\n\n", totalGameSize);
   			rawhid_close(0);
	        return 0;	    	
   		}
	    
	    buffer_flash = (unsigned char*)malloc(totalGameSize); 
	    fread(buffer_flash, 1, totalGameSize, myfile);
		fclose(myfile);
	  
	    printf(" FILESIZE to write: %luKB\n", (totalGameSize/1024));
		
		//1st erase chip !
		printf(" FLASH erase:");
		fflush(stdout);
		verif = 0;
	    buffer_rom = (unsigned char*)malloc(64); //temp for verif

 		while(verif < 0xBF40){     	
		    verif = 0;
	      	HIDcommand[0] = SMS_READ;	//common
	        HIDcommand[1] = 0; 		  	//common
	        
	        HIDcommand[2] = 0;
	      	HIDcommand[3] = 1;
		    HIDcommand[4] = 0;
		    HIDcommand[5] = 0xFD;
		    HIDcommand[6] = 0xFF;
	        rawhid_send(0, HIDcommand, 64, 30);
	        rawhid_recv(0, buffer_rom, 64, 30);
			verif += check_buffer(0, (unsigned char *)buffer_rom, 64);
			
	        HIDcommand[1] = 0x00;
	        HIDcommand[2] = 0x40;
	      	HIDcommand[3] = 1;
		    HIDcommand[4] = 1;
		    HIDcommand[5] = 0xFE;
		    HIDcommand[6] = 0xFF;
	        rawhid_send(0, HIDcommand, 64, 30);
	        rawhid_recv(0, buffer_rom, 64, 30);
			verif += check_buffer(0, (unsigned char *)buffer_rom, 64);
			
	        HIDcommand[1] = 0x00;
	        HIDcommand[2] = 0x80;
	      	HIDcommand[3] = 1;
		    HIDcommand[4] = 2;
		    HIDcommand[5] = 0xFF;
		    HIDcommand[6] = 0xFF;
	        rawhid_send(0, HIDcommand, 64, 30);
	        rawhid_recv(0, buffer_rom, 64, 30);
			verif += check_buffer(0, (unsigned char *)buffer_rom, 64);
	
		    connect = 0;
			HIDcommand[0] = SMS_ERASE_FLASH;
	        rawhid_send(0, HIDcommand, 64, 30);
	        while(connect < 0xFF){
		    	rawhid_recv(0, buffer_recv, 64, 1000);
		    	connect = buffer_recv[0];
	        }
 		}
	    printf(" 100%%\n");
        fflush(stdout);
		free(buffer_rom);

		HIDcommand[0] = SMS_WRITE_FLASH;
		HIDcommand[3] = 1; //enable MAPPER			        
		slotSize = 16384; //16ko - 0x4000
		address = 0;
		verif = 0;    

		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_start = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_start = clock();
		#endif	
		
		while(address<totalGameSize){
        	
        	verif = check_buffer((unsigned long)address, (unsigned char *)buffer_flash, 32);
        	
        	if(check<0x1FE0){ //skip if 32 bytes full of 0xFF
        	
	        	if(address < 0x4000){
	       			slotRegister = 0xFFFD; 	// slot 0 sega
	        		slotAdr = address;     	       		
	        	}else if(address < 0x8000){
	       			slotRegister = 0xFFFE; 	// slot 1 sega
	        		slotAdr = address;     	       		
	        	}else{
	       			slotRegister = 0xFFFF; 	// slot 2 sega
	        		slotAdr = 0x8000 + (address & 0x3FFF);     	
	        	}

	            HIDcommand[1] = slotAdr & 0xFF;					//lo adr
	            HIDcommand[2] = (slotAdr & 0xFF00)>>8; 			//hi adr
		        HIDcommand[4] = (address/slotSize); 			//page MAPPER
		       	HIDcommand[5] = (slotRegister & 0xFF); 			//reg MAPPER lo adr
		        HIDcommand[6] = (slotRegister & 0xFF00)>>8; 	//reg MAPPER hi adr
		        	
				memcpy((unsigned char *)HIDcommand +7, (unsigned char *)buffer_flash +address, 32);
	             
	            rawhid_send(0, HIDcommand, 64, 100);
	            rawhid_recv(0, buffer_recv, 64, 100);
        	}
            address += 32;
            printf("\r FLASH write: %lu%% reg:%04X page:%02X", ((100 * address)/totalGameSize), slotRegister, HIDcommand[4]);
            fflush(stdout);
        }

        printf("\n");

		buffer_rom = (unsigned char*)malloc(totalGameSize);			        
      	HIDcommand[0] = SMS_READ;
      	HIDcommand[3] = 1; //enable MAPPER
		address = 0;
		slotSize = 16384; //16ko - 0x4000

		while(address<totalGameSize){

        	if(address < 0x4000){
       			slotRegister = 0xFFFD; 	// slot 0 sega
        		slotAdr = address;     	       		
        	}else if(address < 0x8000){
       			slotRegister = 0xFFFE; 	// slot 1 sega
        		slotAdr = address;     	       		
        	}else{
        		//sup à 0x7FFF
       			slotRegister = 0xFFFF; 	// slot 2 sega
        		slotAdr = 0x8000 + (address & 0x3FFF);     	
        	}
        		
            HIDcommand[1] = slotAdr & 0xFF;			//lo adr
            HIDcommand[2] = (slotAdr & 0xFF00)>>8; 	//hi adr
	        HIDcommand[4] = (address/slotSize); 			//page MAPPER
	       	HIDcommand[5] = (slotRegister & 0xFF); 			//reg MAPPER lo adr
	        HIDcommand[6] = (slotRegister & 0xFF00)>>8; 	//reg MAPPER hi adr

            rawhid_send(0, HIDcommand, 64, 30);
            rawhid_recv(0, (buffer_rom +address), 64, 30);
            address += 64 ;
            printf("\r FLASH verif: %lu%%", ((100 * address)/totalGameSize));
            fflush(stdout);
        }
        
		#if defined(OS_LINUX) || defined(OS_MACOSX)
			gettimeofday(&ostime, NULL);
			microsec_end = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
			#elif defined(OS_WINDOWS)
			microsec_end = clock();
		#endif
		
		printf("\n-------------------------\n");
		printf(" Pg | Rom    | File   | State\n");
		printf("-------------------------\n");
		while(page<(totalGameSize/slotSize)){
			checksum_rom = 0;
			checksum_flash = 0;
			while(i<(slotSize*(page+1))){
				checksum_rom += buffer_rom[i];
				checksum_flash += buffer_flash[i];
				i++;
			}
			printf(" %02X | %06lX | %06lX ", page, checksum_flash, checksum_rom);
			if(checksum_rom!= checksum_flash){ printf("| BAD!\n");}else{printf("| Good\n");}
			page++;
		}
		printf("-------------------------\n");
    	break;
  
    default:
        printf("\n");
   		free(buffer_rom);
   		rawhid_close(0);
        return 0; 
    }
    
    rawhid_close(0);
    
	#if defined(OS_LINUX) || defined(OS_MACOSX)
		printf(" Time: %lds", (microsec_end - microsec_start)/1000000);
		printf(" (%ldms)\n", (microsec_end - microsec_start)/1000);
		#elif defined(OS_WINDOWS)
		printf(" Time: %lds", (microsec_end - microsec_start)/1000);
		printf(" (%ldms)\n", (microsec_end - microsec_start));
	#endif
	printf("-------------------------\n");

	// checksum each page 
	if(choixMenu<3){
		unsigned int i=0, checksum_rom=0;
		unsigned char page=0, j=0, k=0, l=0;
		
		printf(" CRC32 Rom: %08X \n", crc32(0, buffer_rom, totalGameSize));
		printf("-------------------------\n");
		printf(" Pg | Dump   | Infos\n");
		printf("-------------------------\n");
		while(page<(totalGameSize/slotSize)){
			checksum_rom = 0;
			while(i<(slotSize*(page+1))){
				checksum_rom += buffer_rom[i];
				i++;
			}
			buffer_checksum[page] = checksum_rom;
			printf(" %02X | %06X |", page, checksum_rom);
			
			if(page>0){ //skip 1st page...
				k=0;
				for(j=0; j<page; j++){
		        	if(buffer_checksum[j] == checksum_rom){
		        		k=1;
		        	}
		    	}
		    	if(checksum_rom == 0x27E000|| checksum_rom == 0x3FC000){k=1;} //overdump
			}
			if(k){
				if(l){
					printf(" overdump? %dKB?\n", (l*(slotSize/1024)));
				}
				}else{
				printf("\n");
				l++;
			}
			page++;
		}
		printf("-------------------------\n\n");
	}
    rawhid_close(0);
	free(buffer_rom);
	free(buffer_flash);
    return 0;
}
