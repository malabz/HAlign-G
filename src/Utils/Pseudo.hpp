#pragma once
//¼î»ù±àºÅ
namespace nucleic_acid_pseudo
{

    /*
    constexpr unsigned char GAP = 0;
    constexpr unsigned char C   = 1;
    constexpr unsigned char G   = 2;
    constexpr unsigned char A   = 3;
    constexpr unsigned char T   = 4;
    constexpr unsigned char U   = 4;
    constexpr unsigned char N   = 5; // UNKNOWN
    */
    constexpr unsigned char end_mark = 0;
    constexpr unsigned char GAP = 7;
    constexpr unsigned char A = 0;
    constexpr unsigned char C = 1;
    constexpr unsigned char G = 2;
    constexpr unsigned char T = 3;
    constexpr unsigned char U = 3;
    constexpr unsigned char N = 5; // UNKNOWN

    constexpr unsigned char MAX_ELE = 5;
    constexpr unsigned char NUMBER  = 6;

}

