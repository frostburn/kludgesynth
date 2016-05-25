// Convert key numbers into chromatic key numbers.

int _major_scale(int num)
{
    switch (num) {
        case 0:
            return 0;
        case 1:
            return 2;
        case 2:
            return 4;
        case 3:
            return 5;
        case 4:
            return 7;
        case 5:
            return 9;
        case 6:
            return 11;
        default:
            if (num < 0){
                return _major_scale(num + 7) - 12;
            }
            else {
                return _major_scale(num - 7) + 12;
            }
    }
}

int major_scale(int num, int mod)
{
    return _major_scale(num) + mod;
}

int minor_scale(int num, int mod)
{
    return major_scale(num - 2, mod) + 3;
}

int minor_pentatonic(int num, int mod)
{
    switch (2 * num + !!mod) {
        case 0:
            return 0;
        case 1:
            return -1;
        case 2:
            return 3;
        case 3:
            return 2;
        case 4:
            return 5;
        case 5:
            return 6;
        case 6:
            return 7;
        case 7:
            return 8;
        case 8:
            return 10;
        case 9:
            return 9;
        default:
            if (num < 0) {
                return minor_pentatonic(num + 5, mod) - 12;
            }
            else {
                return minor_pentatonic(num - 5, mod) + 12;
            }
    }
}

int major_pentatonic(int num, int mod)
{
    return minor_pentatonic(num + 1, mod) - 3;
}
