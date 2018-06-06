/* This file is part of MRPI, an example RTM for the PRiME Framework.
 * 
 * MRPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MRPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MRPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Karunakar Basireddy
 */

#include <iostream>
#include "mrpi_to_freq-v1.hpp"
#include<time.h>
#include<sys/time.h>
//#include <cpufreq.h>
#include "phaseprediction.h"
#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <err.h>
//#include "perf_util.h"
//#include "syst_Big.h"
#define SAMPLE_PERIOD 100000000L
#define SCALING_STOP_LENGTH 10

namespace prime
{
namespace rtm_pmc
{

rtm_pmcf::rtm_pmcf(/*unsigned int frame_time*/)
{
    /* e_dl_Env = 1 ;
     e_actwl_Env;
     e_freq_Env = FREQ4 ;
     execute_cc = 0;
     c_dl_Cnt = frame_time;
     c_actwl_Cnt = 0 ;
     c_freq_Cnt = FREQ4 ;
     c_prdwl_Cnt = 0 ;
     c_avgwl_Cnt = 0 ;
     c_sigma_Cnt = SIGMA_DEFAULT ;
     c_random_Cnt = 100 ;
     c_rowNum_Cnt = 0 ;
     c_reward_penalty_Cnt = 0 ;
     c_qTable_value_Cnt = 0 ;
     priority_Cnt = 5;

     Cnt_init();*/
}

rtm_pmcf::~rtm_pmcf() {}

/*void qlearn_sc::run(unsigned int& freq_return, unsigned int cycle_count)
{
	c_actwl_Cnt = cycle_count;
	cnt();
	freq_return = c_freq_Cnt * 1000;
}*/

void rtm_pmcf::run(unsigned int& New_Freq, double MRPI)
{
    //struct timeval  tv1, tv2;
    //gettimeofday(&tv1, NULL);
    static double predicted_MRPI,actual_MRPI;
    double prediction_error=0;
    static double compensated_MRPI_Predicted, Perf_loss, IPS_New, IPS_Old=0,IPS_New_temp;
    static unsigned int Current_Freq;
    // const unsigned int MRPI_Frequency_Table_Big[]= {1100000,1200000,1300000,1400000,1500000,1600000,1700000,1800000,1900000,2000000};
    //Modified for video decode app
    //const unsigned int MRPI_Frequency_Table_Big[]= {1400000,1500000,1600000,1700000,1800000,1900000,2000000};
    const unsigned int MRPI_Frequency_Table_Big[]= {600000,700000,800000,900000,1000000,1100000,1200000,1300000,1400000,1500000,1600000,1700000,1800000,1900000,2000000};

    static int Set_Frequency_Status, Check_No_Change=0;
    /*defining required time parameters*/
    struct timespec tim2,tim3;
    // tim.tv_sec = 0;
    // tim.tv_nsec = SAMPLE_PERIOD;*/
    predicted_MRPI = 0;
    /*get current core operating frequency*/
    // Current_Freq = cpufreq_get_freq_hardware(5);
    //if(Current_Freq!=200000)
    //  Set_Frequency_Status = cpufreq_set_frequency(5,2000000);

    //FILE *fptr;
    // FILE *fptrf;
    // fptr = fopen("MRPI_B.txt","a");
    // fptrf = fopen("Freq_B.txt","a");
    //PMU_initialize();*/
    //while(1)
    //{
    /*Adaptively changing the SAMPLE_PERIOD by checking last N frequencies of operation*/
    //struct timeval  tv1, tv2;
    //gettimeofday(&tv1, NULL);
    if(Check_No_Change<SCALING_STOP_LENGTH)
    {
        /*get perf data before workload prediction starts*/

//	fptr = fopen("MRPI_B.txt","w");
        //     fptrf = fopen("Freq_B.txt","w");


        /* if(fptr==NULL)
          printf("error");
          if(fptrf==NULL)
          printf("error");*/

        // nanosleep(&tim, &tim1);
        actual_MRPI=MRPI;
        //actual_MRPI=Get_PMC_Data_Big();
        //printf("MRPI=%Lf\n",actual_MRPI);	//actual_MRPI = L2D_Cache_Refill/Instructions
        // IPS_New = Instructions/SAMPLE_PERIOD;

        /* Prediction error */
        if(CONSIDER_FEEDBACK)
        {
            prediction_error = actual_MRPI - predicted_MRPI;
        }
        /*prediction starts from here*/
        //printf("B_prediction=%Lf\n",predicted_MRPI);
        predicted_MRPI=Predict_MRPI_Next_Period(predicted_MRPI,actual_MRPI);
        //std::cout<<"Predicted_MRPI"<<predicted_MRPI<<std::endl;
        /*prediction error calculation and feedback*/
        /*
        		if(CONSIDER_FEEDBACK){
        			prediction_error = actual_MRPI - predicted_MRPI;
        		//	printf("Error=%Lf\n",prediction_error);
        		}*/
        compensated_MRPI_Predicted = predicted_MRPI + COMPENSATE_ERROR*prediction_error;
        //printf("MRPI_P=%f\n",compensated_MRPI_Predicted);

        /* Phase to Frequency Mapping : Selecting appropriate frequency of operation */
        
        /* if(compensated_MRPI_Predicted>0.036)//0.036
         {
             New_Freq = MRPI_Frequency_Table_Big[0];
         }
         else if(compensated_MRPI_Predicted>0.033 && compensated_MRPI_Predicted<=0.036)
         {
             New_Freq = MRPI_Frequency_Table_Big[1];
         }
         else if(compensated_MRPI_Predicted>0.03 && compensated_MRPI_Predicted<=0.033)
         {
             New_Freq= MRPI_Frequency_Table_Big[2];
         }
         else if(compensated_MRPI_Predicted>0.027 && compensated_MRPI_Predicted<=0.03)
         {
             New_Freq = MRPI_Frequency_Table_Big[3];
         }
         else if(compensated_MRPI_Predicted>0.024 && compensated_MRPI_Predicted<=0.027)
         {
             New_Freq = MRPI_Frequency_Table_Big[4];
         }
         else if(compensated_MRPI_Predicted>0.021 && compensated_MRPI_Predicted<=0.024)
         {
             New_Freq = MRPI_Frequency_Table_Big[5];
         }
         else if(compensated_MRPI_Predicted>0.018 && compensated_MRPI_Predicted<=0.021)
         {
             New_Freq = MRPI_Frequency_Table_Big[6];
         }
         else if(compensated_MRPI_Predicted>0.015 && compensated_MRPI_Predicted<=0.018)
         {
             New_Freq = MRPI_Frequency_Table_Big[7];
         }
         else if(compensated_MRPI_Predicted>0.012 && compensated_MRPI_Predicted<=0.015)
         {
             New_Freq = MRPI_Frequency_Table_Big[8];
         }
         else if (compensated_MRPI_Predicted>0.009 && compensated_MRPI_Predicted<=0.012)
          {
        New_Freq=MRPI_Frequency_Table_Big[9];
                }
              else
                {*/

        //Modified for video decode app
        if (compensated_MRPI_Predicted>0.01)
        {
            New_Freq=MRPI_Frequency_Table_Big[0];
        }
        else if (compensated_MRPI_Predicted>0.009)
        {
            New_Freq=MRPI_Frequency_Table_Big[1];
        }
        else if (compensated_MRPI_Predicted>0.0083 && compensated_MRPI_Predicted<=0.009)
        {
            New_Freq=MRPI_Frequency_Table_Big[2];
        }
        else if (compensated_MRPI_Predicted>0.0074 && compensated_MRPI_Predicted<=0.0083)
        {
            New_Freq=MRPI_Frequency_Table_Big[3];
        }
        else if (compensated_MRPI_Predicted>0.0063 && compensated_MRPI_Predicted<=0.0074)
        {
            New_Freq=MRPI_Frequency_Table_Big[4];
        }
        else if (compensated_MRPI_Predicted>0.0057 && compensated_MRPI_Predicted<=0.0063)
        {
            New_Freq=MRPI_Frequency_Table_Big[5];
        }
        else if (compensated_MRPI_Predicted>0.0034 && compensated_MRPI_Predicted<=0.0057)
        {
            New_Freq=MRPI_Frequency_Table_Big[6];
        }
        else if (compensated_MRPI_Predicted>0.002 && compensated_MRPI_Predicted<=0.0034)
        {
            New_Freq=MRPI_Frequency_Table_Big[7];
        }
	else if (compensated_MRPI_Predicted>0.0015 && compensated_MRPI_Predicted<=0.002)
        {
            New_Freq=MRPI_Frequency_Table_Big[8];
        }
	else if (compensated_MRPI_Predicted>0.001 && compensated_MRPI_Predicted<=0.0015)
        {
            New_Freq=MRPI_Frequency_Table_Big[9];
        }
	else if (compensated_MRPI_Predicted>0.0095 && compensated_MRPI_Predicted<=0.001)
        {
            New_Freq=MRPI_Frequency_Table_Big[10];
        }
	else if (compensated_MRPI_Predicted>0.0009 && compensated_MRPI_Predicted<=0.0095)
	{
	New_Freq=MRPI_Frequency_Table_Big[11];
	}
	else if (compensated_MRPI_Predicted>0.00085 && compensated_MRPI_Predicted<=0.0009)
	{
	New_Freq=MRPI_Frequency_Table_Big[12];
	}
	else if (compensated_MRPI_Predicted>0.0008 && compensated_MRPI_Predicted<=0.00085)
	{
	New_Freq=MRPI_Frequency_Table_Big[13];
	}
        else
        {
            New_Freq=MRPI_Frequency_Table_Big[14];
        }
        




        /*Set Operating Frequency of the Cores (KHz)*/
        /*if (New_Freq!=Current_Freq)
        {
            Set_Frequency_Status = cpufreq_set_frequency(5,New_Freq);
            if(Set_Frequency_Status!=0){
                err(1,"Failed to set the core frequency to New_Freq: are you root user and have userspace governor installed?");
            }
            Current_Freq=New_Freq;
            Check_No_Change--;
            if(Check_No_Change==-1*(SCALING_STOP_LENGTH))
                Check_No_Change=0;
        }
        else
        {
            Check_No_Change++;
        }*/

        /*Workload modelling and performanace tuning*/
        // if(Check_performance==2)
        /* IPS_New_temp+=IPS_New*0.01;
         if(IPS_New_temp<IPS_Old)
         {
             //	diff=IPS_Old-IPS_New;
             Perf_loss=(IPS_Old-IPS_New)/IPS_Old;
             //compensate the performance loss by stepping up the core frequency
             New_Freq+=Perf_loss*2000000;
             IPS_Old=IPS_New+Perf_loss*IPS_Old;
             Set_Frequency_Status= cpufreq_set_frequency(5,New_Freq);
             if(Set_Frequency_Status!=0)//||Set_Frequency_Status[1]==0||Set_Frequency_Status[2]==0||Set_Frequency_Status[3]==0)
             {
                 err(1,"Failed to set the core frequency to New_Freq: are you root and have userspace governor installed?");
            }
         }*/
        // IPS_Old=IPS_New;
//           printf("%Lf\n",New_Freq);

    }

    /*Stop monitoring if last N (SCALING_STOP_LENGTH) frequencies are same*/

    else
    {
        long double temp1;
        temp1=(long double)(SAMPLE_PERIOD/1000000000);
        temp1=temp1*SCALING_STOP_LENGTH;
        tim2.tv_sec=(int)(temp1);
        if(tim2.tv_sec==0)
        {
            tim2.tv_nsec=(int)(3*SAMPLE_PERIOD);
        }
        else
        {
            tim2.tv_nsec=0;
        }
        nanosleep(&tim2,&tim3);
        Check_No_Change=0;
    }
    /*gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));
    */
    //printf("%Lf\n",New_Freq);

    //}
    // fclose(fptr);
    //fclose(fptrf);


    //PMU_terminate();

    //return 0;
}

/*void qlearn_sc::random_max255(unsigned int *randomvar)
{
    *randomvar = rand();
    *randomvar = *randomvar % 255;
}

void qlearn_sc::random_max18(unsigned int *randomvar)
{
    *randomvar = rand();
    *randomvar = *randomvar % 18;
}

unsigned int qlearn_sc::Env_random_max255(void)
{
    unsigned int x;
    random_max255(&x);
    return x;
}*/

/*unsigned int qlearn_sc::Env_random_frequency(void)
{
    unsigned int x, freq;
    random_max18(&x);
    switch(x)
    {
    case 0:
        freq = FREQ1;
        break;
    case 1:
        freq = FREQ2;
        break;
    case 2:
        freq = FREQ3;
        break;
    case 3:
        freq = FREQ4;
        break;
    case 4:
        freq = FREQ5;
        break;
    case 5:
        freq = FREQ6;
        break;
    case 6:
        freq = FREQ7;
        break;
    case 7:
        freq = FREQ8;
        break;
    case 8:
        freq = FREQ9;
        break;
    case 9:
        freq = FREQ10;
        break;
    case 10:
        freq = FREQ11;
        break;
    case 11:
        freq = FREQ12;
        break;
    case 12:
        freq = FREQ13;
        break;
    case 13:
        freq = FREQ14;
        break;
    case 14:
        freq = FREQ15;
        break;
    case 15:
        freq = FREQ16;
        break;
    case 16:
        freq = FREQ17;
        break;
    case 17:
        freq = FREQ18;
        break;
    case 18:
        freq = FREQ19;
        break;
    default:
        freq = FREQ1;
        break;
    }
    return freq;
}*/

/*void qlearn_sc::Cnt_init(void)
{
    int i0 = 0;
    for (; i0 < ROW; i0++)
    {
        int i1 = 0;
        for (; i1 < COL; i1++)
        {
            c_qTable_Cnt[i0][i1] = 1;
        }
    }
}*/

/*int qlearn_sc::MAXROW(int *a)
{
    int max = -100;
    int i, maxindex = 0;
    for (i = 0; i < COL; ++i)
    {
        if (a[i] > max)
        {
            maxindex = i;
            max = a[i];
        }
    }
    return maxindex;
}*/


/*void qlearn_sc::cnt()
{
    if (c_sigma_Cnt != SIGMA_DEFAULT)   // if MA
    {
        unsigned int tmp;
        c_avgwl_Cnt = ((LAMBDA * c_actwl_Cnt) + ((100-LAMBDA) * c_avgwl_Cnt)) / 100;
        c_rowNum_Cnt = min(ROW - 1,max(0,c_actwl_Cnt / LENGTH - 1));
        if ((((c_actwl_Cnt) / c_freq_Cnt) <= c_dl_Cnt))
        {
            c_reward_penalty_Cnt = min(100,(100 * c_actwl_Cnt) / (c_freq_Cnt * c_dl_Cnt) );
        }
        else
        {
            c_reward_penalty_Cnt = max(-100,- ((((int)c_actwl_Cnt/(int)c_freq_Cnt) - (int)c_dl_Cnt) * 100) / (3 * (int)c_dl_Cnt));
        }

//		std::cout << "c_freq: " << c_freq_Cnt << " c_dl_Cnt: " << c_dl_Cnt << " c_actwl_Cnt: " << c_actwl_Cnt << std::endl;
//		std::cout << "c_reward_penalty_Cnt: " << c_reward_penalty_Cnt << std::endl;

        if ((0 < c_freq_Cnt) && (c_freq_Cnt <= FREQ1))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((1) - (1))] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][1-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ1 < c_freq_Cnt) && (c_freq_Cnt <= FREQ2))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((2) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][2-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ2 < c_freq_Cnt) && (c_freq_Cnt <= FREQ3))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((3) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][3-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ3 < c_freq_Cnt) && (c_freq_Cnt <= FREQ4))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((4) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][4-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ4 < c_freq_Cnt) && (c_freq_Cnt <= FREQ5))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((5) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][5-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ5 < c_freq_Cnt) && (c_freq_Cnt <= FREQ6))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((6) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][6-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ6 < c_freq_Cnt) && (c_freq_Cnt <= FREQ7))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((7) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][7-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ7 < c_freq_Cnt) && (c_freq_Cnt <= FREQ8))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((8) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][8-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ8 < c_freq_Cnt) && (c_freq_Cnt <= FREQ9))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((9) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][9-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ9 < c_freq_Cnt) && (c_freq_Cnt <= FREQ10))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((10) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][10-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ10 < c_freq_Cnt) && (c_freq_Cnt <= FREQ11))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((11) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][11-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ11 < c_freq_Cnt) && (c_freq_Cnt <= FREQ12))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((12) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][12-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ12 < c_freq_Cnt) && (c_freq_Cnt <= FREQ13))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((13) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][13-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ13 < c_freq_Cnt) && (c_freq_Cnt <= FREQ14))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((14) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][14-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ14 < c_freq_Cnt) && (c_freq_Cnt <= FREQ15))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((15) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][15-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ15 < c_freq_Cnt) && (c_freq_Cnt <= FREQ16))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((16) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][16-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ16 < c_freq_Cnt) && (c_freq_Cnt <= FREQ17))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((17) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][17-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else if ((FREQ17 < c_freq_Cnt) && (c_freq_Cnt <= FREQ18))
        {
            c_qTable_Cnt[c_rowNum_Cnt][((18) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][18-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
        else
        {
            c_qTable_Cnt[c_rowNum_Cnt][((19) - 1)] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][19-1]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
        }
    } //MA

    c_prdwl_Cnt = c_avgwl_Cnt;
    c_rowNum_Cnt = min(ROW - 1,max(0,c_prdwl_Cnt / LENGTH - 1));

    c_random_Cnt = Env_random_max255(); //MA

    if ((c_random_Cnt > c_sigma_Cnt) || (c_sigma_Cnt == SIGMA_DEFAULT))
    {
        c_freq_Cnt = Env_random_frequency(); //MA
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((1) - (1))))
    {
        c_freq_Cnt = FREQ1;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((2) - 1)))
    {
        c_freq_Cnt = FREQ2;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((3) - 1)))
    {
        c_freq_Cnt = FREQ3;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((4) - 1)))
    {
        c_freq_Cnt = FREQ4;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((5) - 1)))
    {
        c_freq_Cnt = FREQ5;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((6) - 1)))
    {
        c_freq_Cnt = FREQ6;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((7) - 1)))
    {
        c_freq_Cnt = FREQ7;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((8) - 1)))
    {
        c_freq_Cnt = FREQ8;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((9) - 1)))
    {
        c_freq_Cnt = FREQ9;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((10) - 1)))
    {
        c_freq_Cnt = FREQ10;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((11) - 1)))
    {
        c_freq_Cnt = FREQ11;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((12) - 1)))
    {
        c_freq_Cnt = FREQ12;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((13) - 1)))
    {
        c_freq_Cnt = FREQ13;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((14) - 1)))
    {
        c_freq_Cnt = FREQ14;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((15) - 1)))
    {
        c_freq_Cnt = FREQ15;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((16) - 1)))
    {
        c_freq_Cnt = FREQ16;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((17) - 1)))
    {
        c_freq_Cnt = FREQ17;
    }
    else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == ((18) - 1)))
    {
        c_freq_Cnt = FREQ18;
    }
    else
    {
        c_freq_Cnt = FREQ19;
    }
    c_sigma_Cnt = (c_sigma_Cnt + 1);
}*/
}
}
