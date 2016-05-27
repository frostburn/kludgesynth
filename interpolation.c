// The third order polynomial p(x) with p(0)=y1, p'(0)=y2-y0, p(1)=y2, p'(1)=y3-y1.
double catmull_rom(double x, double y0, double y1, double y2, double y3)
{
    double a3 = y3 - y2 - y0 + y1;
    y3 = y0 - y1 - a3;
    return y1 + x * (y2 - y0 + x * (y3 + x * a3));
}
