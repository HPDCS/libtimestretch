
#define  NORM                  0x7fffffff


unsigned  seed1 = 565534, seed2 = 6789324, seed3 = 436737;
unsigned  seed4 = 42;



double Rand(pseed)      
	unsigned *pseed;
{
   unsigned   temp;
   double     temp1;

   temp = (1220703125 * *pseed) & NORM;
   *pseed = temp;

   temp1 = temp;
   temp1 *= 0.465613e-9;

   return  temp1;

} /* END RAND */


double Expent(unsigned *pseed, float ta)
{
   double   temp;
   unsigned temp1;

	temp1 = *pseed;
	temp = -ta*log(Rand(&temp1));
	*pseed = temp1;

	return(temp);

}/* END EXPENT */



double Random(pseed)      
	unsigned *pseed;
{
   unsigned   temp;
   double     temp1;

   temp = (1220703125 * *pseed) & NORM;
   *pseed = temp;

   temp1 = temp;
   temp1 *= 0.465613e-9;

   return  temp1;

} /* END RAND */

