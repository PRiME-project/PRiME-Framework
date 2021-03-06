/* This file is part of Codegen QLearning, an example RTM for the PRiME Framework.
 * 
 * Codegen QLearning is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Codegen QLearning is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Codegen QLearning.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by James Bantock, Sadegh Dalvandi, Domenico Balsamo, Charles Leech & Graeme Bragg
 */
 
#include <iostream>
#include "codegen.hpp"

//#define DEBUG

namespace prime  { namespace codegen
	{
		
		qlearn_sc::qlearn_sc() {}
		
		qlearn_sc::qlearn_sc(unsigned int frame_time, int row, unsigned int length) 
		{
			e_dl_Env = 1 ;
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
			rowNum = row;
			dlength = length;
			c_qTable_Cnt =  (int**)malloc(rowNum*sizeof(int*));
			for (int i0 = 0; i0 < rowNum; i0++) { 
				c_qTable_Cnt[i0] = (int*)malloc(dlength*sizeof(int));
			}
			
			Cnt_init();
		}
		
		qlearn_sc::~qlearn_sc() {}
		
		//For debugging purposes only
		void qlearn_sc::printTable()
		{
			int i0 = 0; 
			//for (; i0 < ROW; i0++) { 
			for (; i0 < rowNum; i0++) { 
				int i1 = 0; 
				for (; i1 < COL; i1++) { 
					std::cout << c_qTable_Cnt[i0][i1] << ",";
				} 
				
				std::cout << std::endl;
			} 
		}
		
		//void qlearn_sc::run(unsigned int& freq_return, unsigned int cycle_count)
		void qlearn_sc::run(unsigned int& freq_return, unsigned int cycle_count, float d_t)
		{
			c_actwl_Cnt = cycle_count;
			decode_time = d_t * 1000;
			cnt();
			freq_return = c_freq_Cnt * 1000;
			
			//std::cout << "cycle_count," << cycle_count << ", d_t," << d_t << ", freq," << c_freq_Cnt << ", c_dl_Cnt," << c_dl_Cnt << std::endl;
		}
		
		void qlearn_sc::random_max255(unsigned int *randomvar)
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
		}
		
		unsigned int qlearn_sc::Env_random_frequency(void)
		{
			unsigned int x, freq;
			random_max18(&x);
			switch(x){
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
		}
		
		void qlearn_sc::Cnt_init(void)
		{ 
			int i0 = 0; 
			//for (; i0 < ROW; i0++) { 
			for (; i0 < rowNum; i0++) { 
				int i1 = 0; 
				for (; i1 < COL; i1++) { 
					c_qTable_Cnt[i0][i1] = 1; 
				} 
			} 
		} 
		
		int qlearn_sc::MAXROW(int *a)
		{ 
			int max = -100; 
			int i, maxindex = 0; 
			for (i = 0; i < COL; ++i) { 
				if (a[i] > max) { 
					maxindex = i; 
					max = a[i]; 
				} 
			} 
			return maxindex; 
		} 
		
		
		void qlearn_sc::cnt()
		{
			
			int temp = 0;
			if (c_sigma_Cnt != SIGMA_DEFAULT) { //First time the rtm is executed skip this bit and assign a random frequency 

				c_avgwl_Cnt = ((LAMBDA * c_actwl_Cnt) + ((100-LAMBDA) * c_avgwl_Cnt)) / 100;
				//c_rowNum_Cnt = min(ROW - 1,max(0,c_actwl_Cnt / LENGTH - 1));
				c_rowNum_Cnt = min(rowNum - 1,max(0,c_actwl_Cnt / dlength - 1));
								
				// c_dl_Cnt is the deadline set by the application
				// Here we are calculating an approximation of the execution time per frame using
				// the actual freq and the cpu cycle count.
				
				//Is there any better way to get this value? 
				//The way to calculate the reward can be improved considering other inputs, such as power or temperature
				//at this point of the code
				
				//if ((((c_actwl_Cnt) / c_freq_Cnt) <= c_dl_Cnt)) 
				if (decode_time <= c_dl_Cnt)
				{
					//Calculating the reward
					//c_reward_penalty_Cnt = min(100,(100 * c_actwl_Cnt) / (c_freq_Cnt * c_dl_Cnt) ); //positive reward - positive time slack
				    c_reward_penalty_Cnt = min(100,(100 * decode_time) / (c_dl_Cnt) );
				}
				else
				{
					//c_reward_penalty_Cnt = max(-100,- ((((int)c_actwl_Cnt/(int)c_freq_Cnt) - (int)c_dl_Cnt) * 100) / (3 * (int)c_dl_Cnt));
					c_reward_penalty_Cnt = max(-100,- (((decode_time) - (int)(c_dl_Cnt)) * 100) / (3 * (int)(c_dl_Cnt)));
					
					//negative reward - negative time slack
				}
				
				temp = c_rowNum_Cnt;
				
				
				
				//TODO printing the table 
				
				if ((0 < c_freq_Cnt) && (c_freq_Cnt <= FREQ1))
				{
					//Luis uses EWMA strategy to calculate the reward giving more weight to the previous values (0.6) than the actual one 0.4
					c_qTable_Cnt[c_rowNum_Cnt][0] = (((((100) - LEARNING_RATE) * c_qTable_Cnt[c_rowNum_Cnt][0]) + (LEARNING_RATE * c_reward_penalty_Cnt)) / 100);
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
			//c_rowNum_Cnt = min(ROW - 1,max(0,c_prdwl_Cnt / LENGTH - 1));
			c_rowNum_Cnt = min(rowNum - 1,max(0,c_prdwl_Cnt / dlength - 1));
			

			
			c_random_Cnt = Env_random_max255(); //MA
			
			//In order to enable a better exporation of different opersting points
			//we define a random number which influences wherever the best operating point (frequency) is selected
			//increasing the c_sigma_Cnt the number of random choise will reduce. 
			
			if ((c_random_Cnt > c_sigma_Cnt) || (c_sigma_Cnt == SIGMA_DEFAULT))
			{
				c_freq_Cnt = Env_random_frequency(); //MA
			}
			else if ((c_random_Cnt <= c_sigma_Cnt) && (MAXROW(c_qTable_Cnt[c_rowNum_Cnt]) == (0)))
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

#ifdef DEBUG			
			std::cout << "c_avgwl_Cnt, " << c_avgwl_Cnt << " ,c_actwl_Cnt, " << c_actwl_Cnt << ", reward, " << c_reward_penalty_Cnt<< ", row_real_value, " << temp <<", row_pred_value, " << c_rowNum_Cnt << ", freq, "<< c_freq_Cnt << ",decode time," << decode_time  << ",wl/freq, "<< (c_actwl_Cnt) / c_freq_Cnt<< std::endl;
#endif
		}
	}	
}
