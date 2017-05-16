/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: This module provides the network topology. It defines all active links for single experiments.
 */
#include "core/net/mac/prk-mcs/prkmcs.h"

/* activeLinks array defines all active links */
/*
link_t activeLinks[] = {{1, 2}, {2, 1}, {3, 5}, {4, 7}, {5, 3}, {6, 7}, {8, 9}, {9, 8}, {10, 27}, {11, 10}, 
{13, 11}, {14, 13}, {16, 17}, {17, 6}, {18, 16}, {19, 18}, {20, 19}, {21, 4}, {22, 20}, {23, 22}, {26, 23}, {27, 40}, {28, 26}, {29, 28}, 
{31, 32}, {32, 31}, {33, 34}, {34, 21}, {35, 49}, {38, 39}, {39, 38}, {40, 41}, {41, 29}, {46, 47}, 
{47, 33}, {48, 46}, {49, 35}, {50, 66}, {51, 50}, {52, 51}, {53, 48}, {54, 53}, {55, 54}, {58, 57}, {61, 62}, {62, 52}, {65, 66}, 
{69, 55}, {70, 69}, {76, 61}, {78, 79}, {79, 78}, {80, 81}, {81, 80}, {83, 68}, {91, 93}, {92, 76}, 
{93, 92}, {95, 96}, {96, 97}, {99, 97}, {100, 99}, {108, 91}, {110, 117}, {111, 116}, {113, 107}, {114, 108}, 
{115, 117}, {116, 110}, {118, 119}, {119, 118}, {122, 124}, {123, 84}, {124, 70}, {125, 124}, {126, 127}, {127, 84}, {129, 126}};
*/
link_t activeLinks[] = {{4, 19}, {6, 20}, {6, 22}, {7, 22}, {8, 38}, {8, 9}, {9, 38}, {9, 8}, {19, 4}, {20, 35}, {20, 6}, {21, 22}, {21, 52}, {22, 6}, 
{22, 7}, {22, 21}, {24, 8}, {24, 39}, {34, 6}, {34, 19}, {34, 20}, {34, 49}, {35, 36}, {35, 50}, {36, 21}, {36, 22}, {36, 35}, {36, 37}, {37, 36}, 
{37, 52}, {38, 55}, {39, 38}, {39, 40}, {40, 39}, {49, 34}, {50, 35}, {50, 51}, {51, 50}, {51, 52}, {52, 21}, {52, 37}, {52, 51}, {52, 55}, {52, 80}, 
{55, 52}, {61, 77}, {62, 77}, {63, 62}, {64, 79}, {64, 80}, {65, 50}, {65, 80}, {68, 38}, {69, 55}, {70, 53}, {70, 83}, {70, 84}, {77, 61}, {77, 62}, 
{77, 92}, {78, 62}, {79, 52}, {79, 80}, {79, 94}, {80, 52}, {81, 79}, {81, 80}, {81, 82}, {81, 96}, {82, 81}, {82, 83}, {82, 96}, {82, 97}, {83, 53}, 
{83, 70}, {83, 84}, {84, 53}, {84, 70}, {85, 70}, {85, 84}, {85, 100}, {92, 76}, {92, 77}, {92, 112}, {95, 94}, {95, 96}, {96, 82}, {96, 95}, {98, 99}, 
{99, 98}, {99, 128}, {100, 128}, {107, 108}, {107, 112}, {109, 115}, {113, 112}, {115, 109}, {115, 114}, {122, 125}, {123, 125}, {125, 122}, {125, 124}, 
{125, 126}, {126, 99}, {126, 125}, {126, 128}, {127, 99}, {127, 100}, {127, 122}, {127, 126}, {127, 129}, {126, 99}
};

linkaddr_t activeNodes[] = { 9, 8, 24, 40, 39, 38, 55, 53, 70, 69, 68, 85, 84, 83, 100, 99, 98, 128, 126, 124, 129, 127, 125, 123, 122, 97, 
82, 52, 37, 22, 7, 6, 4, 21, 20, 19, 36, 35, 34, 51, 50, 49, 65, 64, 79, 80, 81, 94, 95, 96, 112, 63, 62, 61, 113, 107, 78, 77, 76, 114, 108, 92, 115, 109};
/*
link_t activeLinks[] = {{4, 19}, {6, 20}, {7, 22}, {8, 38}, {21, 52}, {24, 39}, {34, 49}, {35, 36}, {50, 51}, {61, 77}, {64, 79}, {65, 80}, {69, 55}, {125, 124}, {127, 129}, {126, 99},
};
linkaddr_t activeNodes[] = {4, 6, 7, 8, 19, 20, 21, 22, 24, 34, 35, 36, 38, 39, 49, 50, 51, 52, 55, 61, 64, 65, 69, 77, 79, 80, 99, 124, 125, 126, 127, 129};
*/
uint8_t activeLinksSize = sizeof(activeLinks) / sizeof(activeLinks[0]);
uint8_t activeNodesSize = sizeof(activeNodes) / sizeof(activeNodes[0]);

/* local active links table storing the index (in activeLinks table) of links adjoint at this node */
uint8_t localLinks[LOCAL_LINKS_SIZE];
uint8_t localLinksSize;

void topologyInit()
{
	localLinksSize = 0;
	for (uint8_t i = 0; i < activeLinksSize; ++i)
	{
		if (localLinksSize < LOCAL_LINKS_SIZE)
		{
			if (activeLinks[i].sender == node_addr || activeLinks[i].receiver == node_addr)
			{
				localLinks[localLinksSize] = i;
				++localLinksSize;
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			do
			{
				log_error("Local links table is full");
			} while (1);
		}
	}
}

/* find and return the link index */
uint8_t findLinkIndex(linkaddr_t sender, linkaddr_t receiver) 
{
	uint8_t link_index = INVALID_INDEX;
    for(uint8_t i = 0; i < activeLinksSize; i++) {
        if(activeLinks[i].sender == sender && activeLinks[i].receiver == receiver) {
            link_index = i;
	        break;
        }
    }
    return link_index;
}

/* find and return the index of link adjoint at this node */
uint8_t findLinkIndexForLocal(linkaddr_t sender, linkaddr_t receiver)
{
	uint8_t link_index = INVALID_INDEX;
	for (uint8_t i = 0; i < localLinksSize; i++)
	{
		if (activeLinks[localLinks[i]].sender == sender && activeLinks[localLinks[i]].receiver == receiver)
		{
			link_index = localLinks[i];
		}
	}
	return link_index;
}

/* find and return the local index in local link table */
uint8_t findLocalIndex(uint8_t link_index)
{
	uint8_t local_index = INVALID_INDEX;
	for (uint8_t i = 0; i < localLinksSize; ++i)
	{
		if (localLinks[i] == link_index)
		{
			local_index = i;
		}
	}
	return local_index;
}
