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
 
#ifndef CODEGEN_H
#define CODEGEN_H

namespace prime { namespace codegen
{
	typedef int BOOL;
	#define TRUE 1
	#define FALSE 0
	#define FREQ1  200
	#define FREQ2  300
	#define FREQ3  400
	#define FREQ4  500
	#define FREQ5  600
	#define FREQ6  700
	#define FREQ7  800
	#define FREQ8  900
	#define FREQ9  1000
	#define FREQ10  1100
	#define FREQ11  1200
	#define FREQ12  1300
	#define FREQ13  1400
	#define FREQ14  1500
	#define FREQ15  1600
	#define FREQ16  1700
	#define FREQ17  1800
	#define FREQ18  1900
	#define FREQ19  2000
	#define ROW  400
	#define N  19
	#define SIGMA_DEFAULT  80
	#define LEARNING_RATE  40
	#define LENGTH  (250000)
	#define SIGMA_MAX  245
	#define COL  19
	#define LAMBDA  60
	#define RANDOM_MAX  255
	#define CORE1  1
	#define CORE2  2
	#define CORE3  3
	#define CORE4  4
	#define CORE_NUM  4

	#define min(a,b) (((int)(a)<=(int)(b))?(int)(a):(int)(b)) // MA
	#define max(a,b) (((int)(a)>=(int)(b))?(int)(a):(int)(b)) // MA

	class qlearn_sc
	{
	public:
		qlearn_sc(unsigned int frame_time);
		~qlearn_sc();	
		void run(unsigned int& freq_return, unsigned int& core_return, unsigned int cycle_count, unsigned int e_t);
		
	private:
		int e_dl_Env;
		int e_actwl_Env;
		int e_freq_Env;
		unsigned int execute_cc;
		unsigned int c_dl_Cnt; 
		unsigned int  c_actwl_Cnt; 
		unsigned int c_freq_Cnt; 
		unsigned int c_core; 
		unsigned int c_prdwl_Cnt; 
		unsigned int c_avgwl_Cnt; 
		int  c_qTable_Cnt [ROW][COL]; //MC 
		int c_util[1][CORE_NUM];
		int  c_sigma_Cnt; 
		int  c_random_Cnt; 
		int  c_rowNum_Cnt; 
		int  c_reward_penalty_Cnt; 
		int  c_qTable_value_Cnt; 
		int priority_Cnt; 
		int exe_time;
		int c_core_index;
		
		void random_max255(unsigned int *randomvar);
		void random_max18(unsigned int *randomvar);
		unsigned int Env_random_max255(void);
		unsigned int Env_random_frequency(void);
		void Cnt_init(void);
		void cnt();
		int MAXROW(int *a);		
		int MINROW(int *a);		
	};
} }
#endif
