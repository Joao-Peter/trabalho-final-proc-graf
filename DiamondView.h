//
//  SlideView.h
//  ExercSlidemap
//
//  Created by Leandro Tonietto on 09/05/16.
//  Copyright © 2016 Leandro Tonietto. All rights reserved.
//

#ifndef DiamondView_h
#define DiamondView_h

#include "TilemapView.h"
#include <iostream>
using namespace std;

class DiamondView : public TilemapView
{
public:
    void computeDrawPosition(const int col, const int row, const float tw, const float th, float &targetx, float &targety) const
    {
        targetx = col * tw / 2 + row * tw / 2;
        targety = col * th / 2 - row * th / 2;
    }

    void computeTileWalking(int &col, int &row, const int direction) const
    {
        switch (direction)
        {
        case DIRECTION_NORTH:
            col++;
            row--;
            break;
        case DIRECTION_EAST:
            col++;
            row++;
            break;
        case DIRECTION_SOUTH:
            col--;
            row++;
            break;
        case DIRECTION_WEST:
            col--;
            row--;
            break;
        case DIRECTION_NORTHEAST:
            col++;
            break;
        case DIRECTION_SOUTHEAST:
            row++;
            break;
        case DIRECTION_SOUTHWEST:
            col--;
            break;
        case DIRECTION_NORTHWEST:
            row--;
            break;
        }
    }
};

#endif /* DiamondView_h */
