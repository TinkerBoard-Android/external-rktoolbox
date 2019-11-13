#include <stdio.h>
#include <errno.h>
#include <cutils/properties.h>
#include <string.h>
#include <stdlib.h>
#include "handle.h"

#define MODULE_NAME     "deviceinfo"

static void usage(void);
static void dump_info(void);
static int shell(char *cmd, char *result);

static struct {
    char *property;
    char *info;
} system_property[] = {
    {"ro.product.name","Device Name"},
    {"ro.product.model","Device Model"},
    {"ro.target.product","Product Type"},
    {"ro.serialno","Serial Number"},
    {"ro.build.version.release","Android Version"},
    {"ro.build.version.sdk","APILevel"},
    {"ro.build.date","Build Time"},
    {"ro.build.type","Build Type"},
    {"ro.build.version.incremental","Build Version"},
    {"ro.vendor.build.security_patch","Security Patch Level"},
    {"persist.vendor.framebuffer.main","Screen Reslution"},
    {"ro.sf.lcd_density","Screen Density"},
    {"ro.rksdk.version","SDK Version"},
    {"ro.product.cpu.abilist","CPU abi"},
    {"ro.boot.selinux","Selinux"},
    {"ro.boot.storagemedia","Flash type"},
    {0,0},
};

static struct {
    char *cmd;
    char *info;
    void (*func)(char *value);
} system_node[] = {
    {"cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies","CPU Freqs : ", enter_handle},
    {"cat /sys/class/devfreq/*.gpu/available_frequencies","GPU Freqs : ", enter_handle},
    {"cat /sys/class/devfreq/dmc/available_frequencies","DDR Freqs : ", enter_handle},
    {"cat /proc/meminfo | grep MemTotal", "", enter_handle},
//    {"cat /proc/meminfo | grep MemFree", "", enter_handle},
    {0,0, NULL},
};

static struct {
    char *cmd;
    char *info;
    void (*func)(char *value);
} dvfs_node[] = {
    {"cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq","CPU CurFreq :", enter_handle},
    {"cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies","CPU Freqs :", enter_handle},
    {"cat /sys/class/devfreq/*.gpu/cur_freq","GPU CurFreq : ", enter_handle},
    {"cat /sys/class/devfreq/*.gpu/available_frequencies","GPU Freqs :", enter_handle},
    {"cat /sys/class/devfreq/dmc/cur_freq","DDR CurFreq :", enter_handle},
    {"cat /sys/class/devfreq/dmc/available_frequencies","DDR Freqs :", enter_handle},
    //{"cat /sys/kernel/debug/regulator/regulator_summary","Regulator Summary : \n", enter_handle},
    {"cat /d/opp/opp_summary","OPP Summary :\n", enter_handle},
    {0,0, NULL},
};

static int shell(char *cmd, char *result)
{
    FILE *fstream=NULL;
    char buff[1024];
    memset(buff,0,sizeof(buff));
    if(NULL==(fstream=popen(cmd,"r")))
    {
        fprintf(stderr,"execute command failed: %s",strerror(errno));
        return -1;
    }
    while(NULL!=fgets(buff, sizeof(buff), fstream)) {
        strcat(result, buff);
        memset(buff, 0, sizeof(buff));
    }
//        printf("result:%s",result);
    pclose(fstream);
    return 0;
}

static void dump_info(void)
{
    char value[1024 * 10];
    for(int i=0; system_property[i].property; i++)
    {
        property_get(system_property[i].property, value, "");
        printf("%s : %s \r\n",system_property[i].info,value);
        memset(value, 0, sizeof(value));
    }

    for(int i=0; system_node[i].cmd; i++)
    {
        if(shell(system_node[i].cmd, value) != 0)
        {
            sprintf(value," ");
        }
        if(system_node[i].func)
        {
            system_node[i].func(value);
        }

        printf("%s%s \r\n",system_node[i].info, value);
        memset(value, 0, sizeof(value));
    }
}

static void dvfs_info(void)
{
    char value[102400];
    memset(value, 0, sizeof(value));
    for(int i=0; dvfs_node[i].cmd; i++)
    {
        if(shell(dvfs_node[i].cmd, value) != 0)
        {
            sprintf(value," ");
        }

/*        if(dvfs_node[i].func)
        {
            dvfs_node[i].func(value);
        }
*/
        printf("%s%s \r\n",dvfs_node[i].info, value);
        memset(value, 0, sizeof(value));
    }
}

static void usage(void)
{
    printf("Usage:\r\n");
    printf("       deviceinfo  -dump\n");
    printf("       deviceinfo  -dvfs\n");
    printf("       deviceinfo  -help\n");
    printf("\n");
    printf("Miscellaneous:\n");
    printf("  -help       Print help information\n");
    printf("  -dvfs       Dump kernel dvfs info\n");
    printf("  -dump       Dump system info\n");
}

int main(int argc, char **argv)
{

   int i = 0;

    if(argc < 2)
    {
        printf("%s: Need 2 arguments (see \" %s -help\")\n", MODULE_NAME, MODULE_NAME);
        return 0;
    }

#ifdef LOG_DEBUG
   printf("system - argc = %d \r\n",argc);
   for(i=0;i<argc;i++)
   {
       printf("i = %d  value = %s \r\n",i, argv[i]);
   }
#endif

   if(!strcmp(argv[1], "-dump"))
   {
       dump_info();
       return 0;
   }
   else if(!strcmp(argv[1], "-dvfs"))
   { 
       dvfs_info();
       return 0;
   }
   else if(!strcmp(argv[1], "-help"))
   {
       usage();
       return 0;
   }

   printf("%s: no such. (see \" %s -help\")\n", MODULE_NAME, MODULE_NAME);

   return 0;
}
