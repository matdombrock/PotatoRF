#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctime>
#include "ssd1306/ssd1306.h"
#include "GPIO/Button.h"
#include "GPIO/Rotary.h"
#include "GPIO/BitBang.h"

class State{
public:
	int appIndex = -1;
	int tick = 0;
	int utime = (int)time(NULL);
};

class Menu
{
public:
	int drawMenu(int val, bool force = false)
	{
		int font = 1;
		uint8_t rc = 0;
		int newIndex = abs(val / 33) % menuSize;
		if (index == newIndex && force == 0)
		{
			printf("skip\n");
			return rc;
		}
		index = newIndex;
		int indexP = (index - 1) >= 0 ? (index - 1) : (menuSize - 1);
		int indexN = (index + 1) < menuSize ? (index + 1) : 0;
		uint8_t *selected = (uint8_t *)menu[index];
		uint8_t *prev = (uint8_t *)menu[indexP];
		uint8_t *next = (uint8_t *)menu[indexN];
		rc += ssd1306_oled_clear_screen();
		rc += ssd1306_oled_set_XY(0, 0);
		rc += ssd1306_oled_write_line(font, prev);
		rc += ssd1306_oled_set_XY(0, 1);
		rc += ssd1306_oled_write_line(font, (uint8_t *)">>");
		rc += ssd1306_oled_set_XY(16, 1);
		rc += ssd1306_oled_write_line(font, selected);
		rc += ssd1306_oled_set_XY(0, 2);
		rc += ssd1306_oled_write_line(font, next);
		return rc;
	}
	int run(Rotary * rot, Button * btn, bool force = 0){
		uint8_t rc = 0;
		if(force){
			rc += drawMenu(indexRaw, 1);
		}
		std::string rotRead = rot->read();
		if (rotRead.size() > 0)
		{
			for (int i = 0; i < rotRead.size(); i++)
			{
				char dir = rotRead[i];
				if (dir == 'r')
				{
					indexRaw += 1;
				}
				else
				{
					indexRaw += -1;
				}
			}
			printf("r: %i\n", indexRaw);
			rc += drawMenu(indexRaw);
		}
		if (btn->changed())
		{
			printf("btn\n");
			return index;
		}
		return -1;
	}
private:
	int indexRaw = 0;
	int index = 0;
	static const int menuSize = 4;
	char menu[menuSize][16] = {
		"settings",
		"unix time",
		"oled demo",
		"bit banger"};
};

class App{
public:
	App(){};
	~App(){};
	virtual int run(State * st){
		return 0;
	};
};

class App_Hack1: public App
{
public:
	App_Hack1(){};
	int run(State * st) override{
		uint8_t rc = 0;
		int font = 0;
		int ts = (int)time(NULL);
		char msgs[3][16] = {
			"welcome",
			"to the ",
			"machine"
		};
		int i = (ts) % 3;
		if(i == 0 && cleared == false){
			rc += ssd1306_oled_clear_screen();
			cleared = true;
		}
		if(i != 0){
			cleared = false;
		}
		rc += ssd1306_oled_set_XY(i*8, i);
		rc += ssd1306_oled_write_line(font, (uint8_t*)msgs[i]);
		return 0;
	}
private:
	bool cleared = false;
};

class App_UnixTime: public App{
public:
	int run(State * st) override{
		uint8_t rc = 0;
		int font = 1;
		char tss[16];
		sprintf(tss, "%d", st->utime);
		rc += ssd1306_oled_set_XY(0, 0);
		rc += ssd1306_oled_write_line(font, (uint8_t*)">>Unix Time:");
		rc += ssd1306_oled_set_XY(0, 2);
		rc += ssd1306_oled_write_line(font, (uint8_t*)tss);
		return 0;
	}
};

class App_BitBang: public App{
public:
	App_BitBang(BitBang * bitb){
		gpio = bitb;
	}
	int run(State * st) override{
		uint8_t rc = 0;
		int font = 0;
		char tss[16];
		sprintf(tss, "%d", st->utime % 2);
		rc += ssd1306_oled_set_XY(0, 0);
		rc += ssd1306_oled_write_line(font, (uint8_t*)">>Banger:");
		rc += ssd1306_oled_set_XY(0, 2);
		rc += ssd1306_oled_write_line(font, (uint8_t*)tss);
		gpio->set(st->utime % 2);
		return 0;
	}
	BitBang * gpio;
};

int init()
{
	uint8_t rc = 0;
	uint8_t i2c_node_address = 1;
	int font = 0;
	rc = ssd1306_init(i2c_node_address);
	rc += ssd1306_oled_default_config(32, 128);
	rc += ssd1306_oled_clear_screen();
	uint8_t *msg = (uint8_t *)"xX_Zipper_Nero_Xx";
	rc += ssd1306_oled_write_line(font, msg);
	return rc;
}



int main()
{
	State st;
	Menu menu;
	
	Button backBtn("gpiochip1", 83);
	Button btn("gpiochip1", 98);
	Rotary rot(
		"gpiochip0", 9,
		"gpiochip0", 10
	);
	BitBang bitb("gpiochip1", 82);
	//
	App_Hack1 app_hack1;
	App_UnixTime app_unixTime;
	App_BitBang app_bitBang(&bitb);
	//
	
	btn.setToggleMode();
	uint8_t rc = 0;
	rc += init();
	st.appIndex = -1;
	st.tick = 0;
	while (1)
	{
		st.utime = (int)time(NULL);
		//sleep(0.1);
		int font = 0;
		int tempAppIndex = st.appIndex;
		if(backBtn.changed() && st.appIndex != -1){
			printf("back\n");
			// Ensure no rotation carries over
			rot.clear();
			// Reset index
			st.appIndex = menu.run(&rot, &btn, 1);	
		}
		if(st.appIndex == -1){
			tempAppIndex = menu.run(&rot, &btn);
		}
		// Switch apps
		switch(tempAppIndex){
			case 0:
			{
				if(st.appIndex != 0){
					rc += ssd1306_oled_clear_screen();	
				}
				rc += ssd1306_oled_set_XY(0, 0);
				uint8_t *title = (uint8_t *)">> Settings:";
				rc += ssd1306_oled_write_line(font, title);
				break;
			}
			case 1:
			{
				if(st.appIndex != 1){
					rc += ssd1306_oled_clear_screen();	
				}
				app_unixTime.run(&st);
				break;
			}
			case 2:
			{
				if(st.appIndex != 2){
					rc += ssd1306_oled_clear_screen();	
				}
				app_hack1.run(&st);
				break;
			}
			case 3:
			{
				if(st.appIndex != 3){
					rc += ssd1306_oled_clear_screen();	
				}
				app_bitBang.run(&st);
				break;
			}
		}
		st.appIndex = tempAppIndex;
		st.tick++;
	}
	return 0;
}
