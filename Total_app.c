#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <signal.h>
#include "./fpga_dot_font.h"

#include <linux/fb.h>
#include <time.h>

// number of push switch define
#define MAX_BUTTON 9

// TEXT_LCD BUFFER size define
#define MAX_BUFF 32
#define LINE_BUFF 16

#define BUFF_SIZE 1843200 //614400 = 1024*600*3

// using device define
// 1. dot_matrix 2. text_lcd 3. fnd(7segment) 4. buzzer 5. push switch
#define DOT_DEVICE "/dev/fpga_dot"
#define TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define FND_DEVICE "/dev/fpga_fnd"
#define BUZZER_DEVICE "/dev/fpga_buzzer"
#define PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"

//vga_rect
typedef unsigned int U32;
unsigned int makepixel(U32  r, U32 g, U32 b)
{
    return (U32)((r<<16)|(g<<8)|b);
}

typedef struct make_rect {
	U32 pixel;
	int posx1, posx2, posy1, posy2;
} make_rect;

typedef struct temp_num {
	int num;
	U32 pixel;
}temp_num;

// global function
void sigint_handler( int signo);

// global variable number
int count = 41; // game time setting
int gameover_flag = 0;
int timer_flag = 0;

/* fnd(7segment) */
	int fnd;
	unsigned char fnd_data[4] = {'0'-0x30,'0'-0x30,'4'-0x30,'0'-0x30};
	
void sigint_handler( int signo)
{
	unsigned char fnd_reterval;

	fnd_reterval = write(fnd,&fnd_data,4);

	count--;
	
	if(fnd_data[3] == '0'-0x30){
		fnd_data[3] = '9'-0x30;
		fnd_data[2] = fnd_data[2]-1;	
	}
	else
		fnd_data[3] = fnd_data[3]-1;
	
	alarm(1);
	gameover_flag = 0;

	if(count == 3){
		alarm(0);
		gameover_flag = 1;	
	}
}

int main(void)
{
	//vga_rect
	int check;
        int frame_fd;
	U32 pixel_image;
        int offset;
	int posx1, posy1, posx2, posy2;
    	int repx, repy;
    	struct fb_var_screeninfo fvs;
    	make_rect rect[10];
    	U32 rand_pixel[9];
    	int vga_i,vga_j,vga_k;
    	int ran[9];
	int ran_check, ran_next;
	int pick_num[9];
    	srand((unsigned)time(NULL));
	int open_picture;	
	int vga_a;

	U32 main_header[54];
	int open_main;
	U32 main_pixel;
	unsigned char main[BUFF_SIZE];

	temp_num pic[9];
	
	unsigned char picture[BUFF_SIZE];
    	U32 header[56];
	int q = 0;

	U32 done_pixel = makepixel(189,189,189);
	int check_r=0, check_j=0, check_y=0, check_g=0, check_b=0, check_n=0, check_v=0, check_w=0, check_bl=0;

	// dot_matrix //
	int dot_matrix;
	int dot_str_size;
	unsigned char fi_score[3];

	// push_switch //
	int push_i;
	int push_switch;
	int push_buff_size;
	unsigned char push_sw_buff[MAX_BUTTON];
	int button_1 = 0,button_2 = 0,button_3 = 0,button_4 = 0,button_5 = 0,button_6 = 0,button_7 = 0,button_8 = 0,button_9 = 0;

	int button_i = 0;
	int button_j = 0;

	/* text_lcd */
	int text_lcd;
	int str_size_1;
	int str_size_2;
	char str_text1[] = "SURPRISE!!";
	char str_text2[] = "s.o.r.r.y";
	unsigned char string[32];
	
	int text_i;
	
	/* buzzer */
	int buzzer;
	unsigned char buzzer_quit = 0;
	unsigned char buzzer_state = 0;
	unsigned char buzzer_data;
	unsigned char buzzer_retval;
	int buzzer_cnt;

	// variable number init
	int cnt = 0;
	int score = 0;
	dot_str_size=sizeof(fpga_number[score]);
	push_buff_size=sizeof(push_sw_buff);
	//memset(fnd_data, 0, sizeof(fnd_data));
	memset(fi_score,0,sizeof(fi_score));
	memset(push_sw_buff,0,sizeof(push_sw_buff));
	memset(string, 0, sizeof(string));	


	
	//////// driver open //////////

	/* dot matrix */
	dot_matrix = open(DOT_DEVICE, O_WRONLY);
	if(dot_matrix<0){
		printf("Dot MATRIX Device open error : %s\n", DOT_DEVICE);
		exit(1);
	}

	// push switch //
	push_switch=open(PUSH_SWITCH_DEVICE, O_RDWR);
	if(push_switch<0){
		printf("PUSH SWITCH Device Open Error \n");
		close(push_switch);
		return -1;
	}

	/* fnd(7segment) */
	fnd = open(FND_DEVICE, O_RDWR);
	if (fnd<0) {
		printf("FND Device open error : %s\n",FND_DEVICE);
		exit(1);
	}
	
	/* text_lcd */
	text_lcd = open(TEXT_LCD_DEVICE, O_WRONLY);
	if (text_lcd<0) {
		printf("TEXT LCD Device open error : %s\n",TEXT_LCD_DEVICE);
		return -1;
	}
	
	/* buzzer */
	buzzer = open(BUZZER_DEVICE, O_RDWR);
	if (buzzer<0) {
		printf("BUZZER Device open error : %s\n",BUZZER_DEVICE);
		exit(1);
	}

	//vga_rect
	if((frame_fd = open("/dev/fb0",O_RDWR))<0) {
        	perror("Frame Buffer Open Error!");
        	exit(1);
    	}
 
    	if((check=ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0) {
        	perror("Get Information Error - VSCREENINFO!");
        	exit(1);
    	}
 
    	if(fvs.bits_per_pixel != 32) {
        	perror("Unsupport Mode. 32Bpp Only.");
        	exit(1);
    	}
 
   	if(lseek(frame_fd, 0, SEEK_SET) < 0) {  // Set Pixel Map
        	perror("LSeek Error.");
        	exit(1);
    	}

restart: count = 40;
	score = 0;
	fnd_data[0] = '0'-0x30;
	fnd_data[1] = '0'-0x30;
	fnd_data[2] = '4'-0x30;
	fnd_data[3] = '0'-0x30;

	dot_str_size=sizeof(fpga_number[score]);
	push_buff_size=sizeof(push_sw_buff);

	memset(push_sw_buff,0,sizeof(push_sw_buff));
	memset(string, 0, sizeof(string));	

	write(fnd,&fnd_data,4);
	write(dot_matrix,fpga_set_blank,sizeof(fpga_set_blank));
	write(text_lcd,string,MAX_BUFF);

	open_main = open("/root/nfs/TP_OTHERS/main.bmp", O_RDWR);
	if(open_main < 0){
		printf("ERROR : main.bmp open error");
		exit(1);
	}

	read(open_main, main_header, 54);
	read(open_main, main, BUFF_SIZE);
	
	for(vga_i=0; vga_i<600; vga_i++){
		for(vga_j=0; vga_j<1024; vga_j++){
			offset = vga_i*fvs.xres*(32/8)+(1024-vga_j)*(32/8);

			if(lseek(frame_fd, offset, SEEK_SET) < 0){
				printf("ERROR - rect[%d]\n", vga_i);
				perror("LSeek Error! - rect[vga_i]");
				exit(1);
			}

			main_pixel = makepixel(main[BUFF_SIZE-q+2], main[BUFF_SIZE-q+1], main[BUFF_SIZE-q]);
			q = q + 3;
			write(frame_fd, &main_pixel, (32/8));
		}
	}
	// start screen on state
	/*usleep(300);
	read(push_switch, &push_sw_buff,push_buff_size);
	// button inut check!		
	while(!push_sw_buff == 1){};
	for(push_i = 0; push_i<9; push_i++)
	{		
		// button input
		if(push_sw_buff[push_i] == 1) {}
	} */




/*start:	rect[0].pixel = makepixel(255,0,0);	//red
    	rect[1].pixel = makepixel(255,94,0);	//ju
    	rect[2].pixel = makepixel(255,255,0);	//yellow
   	rect[3].pixel = makepixel(0,255,0);	//green
   	rect[4].pixel = makepixel(0,0,255);	//blue
    	rect[5].pixel = makepixel(5,0,153);	//navy
    	rect[6].pixel = makepixel(95,0,255);	//violet
    	rect[7].pixel = makepixel(255,255,255);	//white
    	rect[8].pixel = makepixel(0,0,0);	//black
    
    	rect[0].posx1 = rect[3].posx1 = rect[6].posx1 = 0;
    	rect[0].posx2 = rect[3].posx2 = rect[6].posx2 = 340;

    	rect[1].posx1 = rect[4].posx1 = rect[7].posx1 = 341;
    	rect[1].posx2 = rect[4].posx2 = rect[7].posx2 = 681;

    	rect[2].posx1 = rect[5].posx1 = rect[8].posx1 = 682;
    	rect[2].posx2 = rect[5].posx2 = rect[8].posx2 = 1024;


    	rect[0].posy1 = rect[1].posy1 = rect[2].posy1 = 0;
    	rect[0].posy2 = rect[1].posy2 = rect[2].posy2 = 199;

    	rect[3].posy1 = rect[4].posy1 = rect[5].posy1 = 200;
    	rect[3].posy2 = rect[4].posy2 = rect[5].posy2 = 399;

    	rect[6].posy1 = rect[7].posy1 = rect[8].posy1 = 400;
    	rect[6].posy2 = rect[7].posy2 = rect[8].posy2 = 600;
	

    	for(vga_i=0; vga_i<9; vga_i++){
		ran[vga_i] = rand()%9;
		rand_pixel[vga_i] = rect[ran[vga_i]].pixel;
    	}

	for(vga_i=0; vga_i<9; vga_i++){
		for(repy=rect[vga_i].posy1; repy<rect[vga_i].posy2; repy++){
			offset = repy*fvs.xres*(32/8)+rect[vga_i].posx1*(32/8);

			if(lseek(frame_fd, offset, SEEK_SET) < 0){
				printf("ERROR - rect[%d]\n", vga_i);
				perror("LSeek Error! - rect[vga_i]");
				exit(1);
			}
			
			for(repx=rect[vga_i].posx1; repx<=rect[vga_i].posx2; repx++){
				write(frame_fd, &rand_pixel[vga_i], (32/8));
			}
		}
	}

	for(vga_i=0; vga_i<9; vga_i++){
		pic[vga_i].num = vga_i;
		pic[vga_i].pixel = rand_pixel[vga_i];

		if(rect[0].pixel == rand_pixel[vga_i]) check_r = check_r + 1;		//red
		else if(rect[1].pixel == rand_pixel[vga_i]) check_j = check_j + 1;	//ju
		else if(rect[2].pixel == rand_pixel[vga_i]) check_y = check_y + 1;	//yellow
		else if(rect[3].pixel == rand_pixel[vga_i]) check_g = check_g + 1;	//green
		else if(rect[4].pixel == rand_pixel[vga_i]) check_b = check_b + 1;	//blue
		else if(rect[5].pixel == rand_pixel[vga_i]) check_n = check_n + 1;	//navy
		else if(rect[6].pixel == rand_pixel[vga_i]) check_v = check_v + 1;	//violet
		else if(rect[7].pixel == rand_pixel[vga_i]) check_w = check_w + 1;	//white
		else if(rect[8].pixel == rand_pixel[vga_i]) check_bl = check_bl + 1;	//black
	}*/
	
	/*if(check_r + check_j + check_y + check_g + check_b + check_n + check_v + check_w + check_bl != 9) exit(1);
	vga_i = 0;*/
	

	// start screen over
	if(button_i<1)
	{
		printf("start_screen!!\n");
		memset(push_sw_buff,0,sizeof(push_sw_buff));
		// start screen on state
		usleep(300);
		read(push_switch, &push_sw_buff,push_buff_size);
		// button inut check!		
		for(push_i = 0; push_i<9; push_i++)
		{		
			// button input
			if(push_sw_buff[push_i] == 1) {
				printf(" input button, GAME method \n ");
				button_i++;
			}

			//while(push_sw_buff[push_i] != 1);
		}

	}	

start:	rect[0].pixel = makepixel(255,0,0);	//red
    	rect[1].pixel = makepixel(255,94,0);	//ju
    	rect[2].pixel = makepixel(255,255,0);	//yellow
   	rect[3].pixel = makepixel(0,255,0);	//green
   	rect[4].pixel = makepixel(0,0,255);	//blue
    	rect[5].pixel = makepixel(5,0,153);	//navy
    	rect[6].pixel = makepixel(95,0,255);	//violet
    	rect[7].pixel = makepixel(255,255,255);	//white
    	rect[8].pixel = makepixel(0,0,0);	//black
    
    	rect[0].posx1 = rect[3].posx1 = rect[6].posx1 = 0;
    	rect[0].posx2 = rect[3].posx2 = rect[6].posx2 = 340;

    	rect[1].posx1 = rect[4].posx1 = rect[7].posx1 = 341;
    	rect[1].posx2 = rect[4].posx2 = rect[7].posx2 = 681;

    	rect[2].posx1 = rect[5].posx1 = rect[8].posx1 = 682;
    	rect[2].posx2 = rect[5].posx2 = rect[8].posx2 = 1024;


    	rect[0].posy1 = rect[1].posy1 = rect[2].posy1 = 0;
    	rect[0].posy2 = rect[1].posy2 = rect[2].posy2 = 199;

    	rect[3].posy1 = rect[4].posy1 = rect[5].posy1 = 200;
    	rect[3].posy2 = rect[4].posy2 = rect[5].posy2 = 399;

    	rect[6].posy1 = rect[7].posy1 = rect[8].posy1 = 400;
    	rect[6].posy2 = rect[7].posy2 = rect[8].posy2 = 600;
	

    	for(vga_i=0; vga_i<9; vga_i++){
		ran[vga_i] = rand()%9;
		rand_pixel[vga_i] = rect[ran[vga_i]].pixel;
    	}

	for(vga_i=0; vga_i<9; vga_i++){
		for(repy=rect[vga_i].posy1; repy<rect[vga_i].posy2; repy++){
			offset = repy*fvs.xres*(32/8)+rect[vga_i].posx1*(32/8);

			if(lseek(frame_fd, offset, SEEK_SET) < 0){
				printf("ERROR - rect[%d]\n", vga_i);
				perror("LSeek Error! - rect[vga_i]");
				exit(1);
			}
			
			for(repx=rect[vga_i].posx1; repx<=rect[vga_i].posx2; repx++){
				write(frame_fd, &rand_pixel[vga_i], (32/8));
			}
		}
	}

	for(vga_i=0; vga_i<9; vga_i++){
		pic[vga_i].num = vga_i;
		pic[vga_i].pixel = rand_pixel[vga_i];

		if(rect[0].pixel == rand_pixel[vga_i]) check_r = check_r + 1;		//red
		else if(rect[1].pixel == rand_pixel[vga_i]) check_j = check_j + 1;	//ju
		else if(rect[2].pixel == rand_pixel[vga_i]) check_y = check_y + 1;	//yellow
		else if(rect[3].pixel == rand_pixel[vga_i]) check_g = check_g + 1;	//green
		else if(rect[4].pixel == rand_pixel[vga_i]) check_b = check_b + 1;	//blue
		else if(rect[5].pixel == rand_pixel[vga_i]) check_n = check_n + 1;	//navy
		else if(rect[6].pixel == rand_pixel[vga_i]) check_v = check_v + 1;	//violet
		else if(rect[7].pixel == rand_pixel[vga_i]) check_w = check_w + 1;	//white
		else if(rect[8].pixel == rand_pixel[vga_i]) check_bl = check_bl + 1;	//black
	}




	if(button_j<1)
	{
		memset(push_sw_buff,0,sizeof(push_sw_buff));
		// game method
		
		usleep(300);
		read(push_switch, &push_sw_buff,push_buff_size);
		// button inut check!		
		for(push_i = 0; push_i<9; push_i++)
		{		
			// button input
			if(push_sw_buff[push_i] == 1) {
				printf(" input button, GAME START! \n");
				button_j++;
				timer_flag = 1;
			}
		}	
	}

//	if(timer_flag)
//	{
		//////////////// TIMER //////////////////////////////
		signal(SIGALRM, sigint_handler);
		alarm(1); // call alarm() - every one sec
//	}

	while(!(gameover_flag)){
	// Play Game!				
		usleep(300);
		read(push_switch, &push_sw_buff,push_buff_size);
//		button_1 = push_sw_buff[0];
//		button_2 = push_sw_buff[1];
//		button_3 = push_sw_buff[2];
//		button_4 = push_sw_buff[3];
//		button_5 = push_sw_buff[4];
//		button_6 = push_sw_buff[5];
//		button_7 = push_sw_buff[6];
//		button_8 = push_sw_buff[7];
//		button_9 = push_sw_buff[8];
		
		for(push_i = 0; push_i<9; push_i++)
		{		
			if(push_sw_buff[push_i] == 1) {
				
				printf("input button : %d\n",(push_i+1));
				cnt++;

				// color compare
				if(check_r > 0){
					printf("red\n");
					if(pic[push_i].pixel == rect[0].pixel){
						printf("correct!\n");
						check_r = check_r - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_j > 0){
					printf("j\n");
					if(pic[push_i].pixel == rect[1].pixel){
						printf("correct!\n");
						check_j = check_j - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_y > 0){
					printf("yello\n");
					if(pic[push_i].pixel == rect[2].pixel){
						printf("correct!\n");
						check_y = check_y - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_g > 0){
					printf("g\n");
					if(pic[push_i].pixel == rect[3].pixel){
						printf("correct!\n");
						check_g = check_g - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_b > 0){
					printf("b\n");
					if(pic[push_i].pixel == rect[4].pixel){
						printf("correct!\n");
						check_b = check_b - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_n > 0){
					printf("n\n");
					if(pic[push_i].pixel == rect[5].pixel){
						printf("correct!\n");
						check_n = check_n - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_v > 0){
					if(pic[push_i].pixel == rect[6].pixel){
						printf("correct!\n");
						check_v = check_v - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_w > 0){
					if(pic[push_i].pixel == rect[7].pixel){
						printf("correct!\n");
						check_w = check_w - 1;
						score = score + 1;
					}
					else printf("wrong!\n");
				}
				else if(check_bl > 0){
					if(pic[push_i].pixel == rect[8].pixel){
						printf("correct!\n");
						check_bl = check_bl - 1;
						score = score + 1;
						if(check_bl == 0) goto start;
					}
					else printf("wrong!\n");
				}
				
				write(dot_matrix,fpga_number[score],dot_str_size);
				sleep(1);
			}

		}	
		
	}
	
	// GAME OVER!
	if(gameover_flag == 1 )
	{	
		printf("GAME OVER!\n");
		// TEXT_LCD
	
		// write str_text1 : first line
		str_size_1 = strlen(str_text1);
		if(str_size_1>0){
			strncat(string, str_text1,str_size_1);
			memset(string+str_size_1, ' ', LINE_BUFF-str_size_1);
		}

		//write str_text2 : second line
		str_size_2 = strlen(str_text2);
		if(str_size_2>0){
			strncat(string, str_text2, str_size_2);
			memset(string+LINE_BUFF+str_size_2, ' ', LINE_BUFF-str_size_2);
		}
	
		// BUZZER on
		// scare picture on!
		open_picture = open("/root/nfs/TP_OTHERS/scare1.bmp", O_RDWR);
	    	if(open_picture < 0) {
			printf("Error : dog1.bmp not open\n");
			exit(1);
	   	 }
	    
	    	read(open_picture, header, 54); //read header

	    	read(open_picture, picture, BUFF_SIZE);

		posx1 = posy1 = 0;
		posx2 = 1024;
		posy2 = 600;

		vga_a = vga_j = vga_i = 0;

		for(vga_a=0; vga_a<600; vga_a++){
			for(vga_j=0; vga_j<1024; vga_j++){		
				offset = (vga_a)*fvs.xres*(32/8)+(1024-vga_j)*(32/8);
				if(lseek(frame_fd, offset, SEEK_SET) < 0){
					perror("LSeek ERROR! : offset");
					exit(1);
				}
	
				pixel_image = makepixel(picture[BUFF_SIZE-vga_i+2], picture[BUFF_SIZE-vga_i+1], picture[BUFF_SIZE-vga_i]);
				vga_i = vga_i + 3;
				write(frame_fd, &pixel_image, (32/8));
				write(text_lcd,string,MAX_BUFF);
			}
		}
		for(buzzer_cnt=0; buzzer_cnt<6; buzzer_cnt++)
		{
			if(buzzer_state != 0) {
				buzzer_state = 0;					
				buzzer_data = 1;				
				buzzer_retval = write(buzzer, &buzzer_data,1);
			
				if(buzzer_retval<0) {
					printf("buzzer write Error!\n");
					return -1;
				}							
			}
			else {
				buzzer_state = 1;
				buzzer_data = 0;
					
				buzzer_retval = write(buzzer, &buzzer_data,1);
				if(buzzer_retval<0) {
				printf("buzzer write Error!\n");
				return -1;
				}
			}
			sleep(1);
		}
	
	
		buzzer_data = 0;
		buzzer_retval = write(buzzer, &buzzer_data,1);
		if(buzzer_retval<0) {
			printf("buzzer write Error!\n");
			return -1;
		}
		gameover_flag == 0;
		goto restart;
	}

	/////// driver close /////////
	close(dot_matrix);
	close(push_switch);
	close(fnd);
	close(text_lcd);
	close(buzzer);	
	close(frame_fd);

	return(0);
}
