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

link_t activeLinks[] = { {70, 85}, {70, 84}, {84, 85}, {83, 84}, {85, 100}, {85, 99}, {99, 84}, {83, 84}, {83, 98}, {98, 97}, {83, 97}, {82, 97}, {81, 96},
{96, 122}, {96, 81}, {82, 81}, {124, 122}, {124, 98}, {122, 125}, {128, 129}, {129, 127}, {126, 123}};

uint8_t activeLinksSize = sizeof(activeLinks) / sizeof(activeLinks[0]);

/* local active links table storing the index (in activeLinks table) of links adjoint at this node */
uint8_t localLinks[LOCAL_LINKS_SIZE];
uint8_t localLinksSize;

void topologyInit()
{
	localLinksSize = 0;
	for (uint8_t i = 0; i < activeLinksSize; i++)
	{
		if (activeLinks[i].sender == node_addr || activeLinks[i].receiver == node_addr)
		{
			localLinks[localLinksSize++] = i;
		}
	}
}

/* check if a <sender, receiver> is active */
uint8_t isActiveLink(linkaddr_t sender, linkaddr_t receiver) {
    for(uint8_t i = 0; i < activeLinksSize; i++) {
        if(activeLinks[i].sender == sender && activeLinks[i].receiver == receiver) {
            return 1;
        }
    }
    return 0;
}

/* find and return the link index */
uint8_t findLinkIndex(linkaddr_t sender, linkaddr_t receiver) {
    for(uint8_t i = 0; i < activeLinksSize; i++) {
        if(activeLinks[i].sender == sender && activeLinks[i].receiver == receiver) {
            return i;
        }
    }
    return INVALID_INDEX;
}

/* find and the return the index of link adjoint at this node */
// Note either @sender or @receiver should be node_addr
uint8_t findLinkIndexForLocal(linkaddr_t sender, linkaddr_t receiver)
{
	for (uint8_t i = 0; i < localLinksSize; i++)
	{
		if (activeLinks[localLinks[i]].sender == sender && activeLinks[localLinks[i]].receiver == receiver)
		{
			return localLinks[i];
		}
	}
	return INVALID_INDEX;
}

uint8_t findLocalIndex(uint8_t link_index)
{
	for (uint8_t i = 0; i < localLinksSize; ++i)
	{
		if (localLinks[i] == link_index)
		{
			return i;
		}
	}
	return INVALID_INDEX;
}
