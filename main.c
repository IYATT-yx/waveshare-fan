/**
 * @file main.c
 * @author IYATT-yx
 * @brief 树莓派 cm4 waveshare 底板控制程序
 * @version 0.1
 * @date 2022-05-26
 * 
 * @copyright Copyright (C) 2022 IYATT-yx iyatt@iyatt.com
	每个人都可以复制和分发本许可证文档的逐字副本，但不允许更改。
	这个程序是自由软件：你可以在自由软件基金会发布的GNU Affro通用公共许可证的条款下重新分发它和/或修改它，
	或者许可证的第3版，或者（在你的选择）任何其他版本。
	本程序的发布目的是希望它有用，但不提供任何担保；甚至没有对适销性或特定用途适用性的默示保证。
	有关更多详细信息，请参阅GNU Affero通用公共许可证。
	您应该已经收到GNU Affero通用公共许可证的副本以及本程序。如果没有，请参阅<https://www.gnu.org/licenses/>.
	Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.
	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */
#include "ssd1306_i2c.h"
#include "DEV_Config.h"
#include "Waveshare_EMC2301.h"
#include "wiringPi.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <dirent.h>

long g_pre_idle, g_pre_total, g_user, g_nice, g_sys, g_idle, g_iowait, g_irq, g_softirq, g_total;
char g_cpu_stat_string[64];
/**
 * @brief 获取 CPU 使用率
 * 
 * @return int CPU 使用率数值
 */
int get_cpu_usage()
{
    FILE *cpu_stat_file = fopen("/proc/stat", "r");
    memset(g_cpu_stat_string, '\0', sizeof(g_cpu_stat_string));
    fread(g_cpu_stat_string, sizeof(char), sizeof(g_cpu_stat_string), cpu_stat_file);
    fclose(cpu_stat_file);

    sscanf(g_cpu_stat_string, "%*s %ld %ld %ld %ld %ld %ld %ld", &g_user, &g_nice, &g_sys, &g_idle, &g_iowait, &g_irq, &g_softirq);
    g_total = g_user + g_nice + g_sys + g_idle + g_iowait + g_irq + g_softirq;
    int usage = (int)((double)(g_total - g_pre_total - (g_idle - g_pre_idle)) / (double)(g_total - g_pre_total) * 100);
    g_pre_total = g_total;
    g_pre_idle = g_idle;

    return usage;
}

char g_cpu_temperature_string[6];
/**
 * @brief 获取 CPU 温度
 * 
 * @return int CPU 温度值，摄氏度
 */
int get_cpu_temperature()
{
    FILE *cpu_temperature_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	memset(g_cpu_temperature_string, '\0', sizeof(g_cpu_temperature_string));
    fread(g_cpu_temperature_string, sizeof(char), sizeof(g_cpu_temperature_string), cpu_temperature_file);
    fclose(cpu_temperature_file);

    long temperature;
    sscanf(g_cpu_temperature_string, "%ld", &temperature);
	temperature /= 1000;

    return (int)temperature;
}

struct ifaddrs *g_ifAddrStruct = NULL;
void *g_tmpAddrPtr = NULL;
char g_ip[24];
/**
 * @brief 获取 IP
 * 
 * @return char* eth0/wlan0:IP
 */
char *get_ip()
{
    getifaddrs(&g_ifAddrStruct);
    while (g_ifAddrStruct != NULL)    
    {
        if (g_ifAddrStruct->ifa_addr->sa_family == AF_INET)
        {
            g_tmpAddrPtr = &((struct sockaddr_in *)g_ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, g_tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

			memset(g_ip, '\0', sizeof(g_ip));
			if (strcmp(g_ifAddrStruct->ifa_name, "eth0") == 0)
			{
				sprintf(g_ip, "eth0:%s", addressBuffer);
				break;
			}
			else if (strcmp(g_ifAddrStruct->ifa_name, "wlan0") == 0)
			{
				sprintf(g_ip, "wlan0:%s", addressBuffer);
				break;
			}
        }
        g_ifAddrStruct = g_ifAddrStruct->ifa_next;
    }
	return g_ip;
}

char g_ram[32];
static struct sysinfo g_sys_info;
unsigned long g_total_ram, g_free_ram;
/**
 * @brief 获取内存可用大小
 * 
 * @return char* RAM:空闲/总MB
 */
char *get_ram()
{
	memset(g_ram, '\0', sizeof(g_ram));
	sysinfo(&g_sys_info);
	g_total_ram = g_sys_info.totalram >> 20;
	g_free_ram = g_sys_info.freeram >> 20;
	sprintf(g_ram, "RAM:%ld/%ldMB", g_free_ram, g_total_ram);
	return g_ram;
}

char g_disk[24];
static struct statfs g_disk_struct;
long long unsigned g_total_disk;
long long unsigned g_free_disk;
/**
 * @brief 获取磁盘使用信息
 * 
 * @return char* Disk:空闲磁盘空间/总磁盘空间MB
 */
char *get_disk()
{
	statfs("/", &g_disk_struct);
	g_total_disk = g_disk_struct.f_bsize * g_disk_struct.f_blocks >> 20;
	g_free_disk = g_disk_struct.f_bsize * g_disk_struct.f_bfree >> 20;
	memset(g_disk, '\0', sizeof(g_disk));
	sprintf(g_disk, "Disk: %lld/%lld MB", g_free_disk, g_total_disk);
	return g_disk;
}

/**
 * @brief oled 按行显示，共四行
 * 
 * @param row 显示位置行数
 * @param s 显示内容字符串
 */
void write_line_string(int row, const char *s)
{
	int i = 0;
	while (s[i] != '\0')
	{
		ssd1306_drawChar(i * 6, row * 9, s[i], 1, 1);
		++i;
	}
}


int main()
{
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);  // oled 初始化
	// // 风扇初始化
    DEV_ModuleInit();
    DEV_I2C_Init(EMC2301_ADDRESS);
	EMC2301_setSpinUpDrive(60);
	EMC2301_setSpinUpTime(300);
	EMC2301_setDriveUpdatePeriod(100);
	EMC2301_RPMEnable();
	int product_ID = I2C_Read_Byte(PRODUCT_ID);
	int Manufacturer_ID = I2C_Read_Byte(MANUFACTURER_ID);
	if(product_ID == 0x37&& Manufacturer_ID==0x5D)
	{
		printf("风扇初始化成功！\n");
		EMC2301_start();
	}

	while (1)
	{
		// // 风扇温控调速
		int temp = get_cpu_temperature();
		int speed_grade = (temp - 40) * 16 - 1;
		if (speed_grade > 255)
		{
			speed_grade = 255;
		}
		else if (speed_grade < 0)
		{
			speed_grade = 0;
		}
		EMC2301_Directspeedcontrol(speed_grade);

		ssd1306_clearDisplay();  // 清空 oled 缓存

		// // 第一行显示 CPU 使用率
		static char row0[10];
		memset(row0, '\0', sizeof(row0));
		sprintf(row0, "CPU: %d%%", get_cpu_usage());
		write_line_string(0, row0);

		// // 第二行显示温度
		static char row1[10];
		memset(row0, '\0', sizeof(row1));
		sprintf(row1, "Temp: %dC", temp);
		write_line_string(1, row1);

		// // 第三行显示风速等级
		// // 第四行显示实际风扇转速
		static char row2[11];
		static char row3[13];
		memset(row2, '\0', sizeof(row2));
		memset(row3, '\0', sizeof(row3));
		sprintf(row2, "Grade: %d", speed_grade);
		sprintf(row3, "Speed: %d", EMC2301_fetchFanSpeed());
		write_line_string(2, row2);
		write_line_string(3, row3);
		// 第五行显示内存使用情况
		write_line_string(4, get_ram());
		// 第六行显示磁盘使用情况
		write_line_string(5, get_disk());
		// 第七行显示 IP
		write_line_string(6, get_ip());

		ssd1306_display();  // 刷新缓存到屏幕
		delay(3000);  // 重复间隔时间
	}
	DEV_ModuleExit();
}
