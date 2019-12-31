#include<reg51.h>
#include"lcd.h"
#include "516_input.h"
#include<math.h>
#include "extend_rom.h"
#define pushStack(stk, target, _char) stk.num[++stk.top].payload = (float)target; \
														stk.num[stk.top].isChar = _char;
#define popStack(stk) (--stk.top)
#define topStack(stk) (stk.num[stk.top])
#define resetStack(stk) (stk.top = -1)
#define StackIsEmpty(stk) (stk.top < 0)
#define OpLevel(c) (c == '+' || c == '-' ? 1 : (c == '*' || c == '/' ? 3 : (c == 'm' || c == '$' ? 2 : 0)))
#define IsNumber(c) (c >= '0' && c <= '9')
#define OneOp(c) (c == 'm' || c == '$' || c == 'u')
#define __CALC__

// used to store the final result									
float result;

unsigned char iterate_lcd_ram(unsigned char read_pos)
{
	unsigned char _data;

	if ( read_pos > 0xA7 || read_pos < 0x80)
		return 0;
	 
	LcdWriteCom( read_pos );
	_data = ReadLcdData();

	if ( _data == 0x20 )
		return 0; 
	
	return _data;
}

typedef struct Node {
    float payload;
    char isChar;
} Node;

typedef struct {
    char top;
    Node num[15];
}Stk;


float Q_sqrt(float z)
{
    unsigned long b;
    float g;
    b = *(unsigned long *) &z;

    b = (b >> 1) + 0x1fbffb72;  //wtf
    g = *(float *) &b;
    g = (.5 * g + .5 * z / g);
    g = (.5 * g + .5 * z / g);
    return g;
}

void rePolish(Stk* stk) {
    Stk s;
	bit isNum = 0;
	char t;
	unsigned char str = 0x80;
    resetStack(s);
    for( ; iterate_lcd_ram( str ) ; str++) {
        t = iterate_lcd_ram( str );
		if ( t == 0 )
			break;
        else if (t == '(') {
            pushStack(s, t, 1);
        }
        else if (t == ')') {
            while (topStack(s).isChar && topStack(s).payload != '(') {
                pushStack((*stk), topStack(s).payload, 1);
                popStack(s);
            }
            popStack(s);
            while(!StackIsEmpty(s) && topStack(s).isChar && OneOp(topStack(s).payload)) {
                pushStack((*stk), topStack(s).payload, 1);
                popStack(s);
            }
        } else if (t == '$') {
            pushStack(s, '$', 1);
        } else if (t == 'u') {
            pushStack(s, 'u', 1);
        }
        else if (t == '+' || t == '*' || t == '/') {
            while (OpLevel(topStack(s).payload) >= OpLevel(t)) {
                pushStack((*stk), topStack(s).payload, 1);
                popStack(s);
            }
            pushStack(s, t, 1);
        } else if (t == '-') {
            if (isNum) {
                while (OpLevel(topStack(s).payload) >= OpLevel(t)) {
                    pushStack((*stk), topStack(s).payload, 1);
                    popStack(s);
                }
                pushStack(s, t, 1);
            }
            else {
                pushStack(s, 'm', 1);
            }
        } else {
            float strnum = 0;
			float loadBit;
            while (IsNumber(iterate_lcd_ram( str ))) {
                isNum = 1;
                strnum *= 10;
                strnum += iterate_lcd_ram( str ) - '0';
                ++str;
            }
            if (iterate_lcd_ram( str ) == '.') {
                ++str;
                loadBit = 0.1f;
                while(IsNumber(iterate_lcd_ram( str ))) {
                    strnum = strnum + (iterate_lcd_ram( str ) - '0') * loadBit;
                    loadBit *= 0.1;
                    ++str;
                }
            }
            pushStack((*stk), strnum, 0);
            --str;
        }
    }
    while (!StackIsEmpty(s)) {
        pushStack((*stk), topStack(s).payload, topStack(s).isChar);
        popStack(s);
    }
}

float calc(Stk* stk) {
    Stk s;
	float tmp;
    resetStack(s);
    while (!StackIsEmpty((*stk))) {
        //Node top = topStack((*stk));
        if (topStack((*stk)).isChar && topStack((*stk)).payload == '+') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = tmp + topStack(s).payload;
            popStack(s);
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == '-') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = topStack(s).payload - tmp;
            popStack(s);
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == '*') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = tmp * topStack(s).payload;
            popStack(s);
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == '/') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = topStack(s).payload / tmp;
            popStack(s);
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == 'm') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = -tmp;
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == '$') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = Q_sqrt(tmp);
        }
        else if (topStack((*stk)).isChar && topStack((*stk)).payload == 'u') {
            tmp = topStack(s).payload;
            popStack(s);
            tmp = pow(tmp, 1.0 / 3);
        }
        else {
            tmp = topStack((*stk)).payload;
        }
        popStack((*stk));
        pushStack(s, tmp, 0);
    }
		
		if( StackIsEmpty(s)) {
			return 0;
		}
    return topStack(s).payload;
}

void reverseStack(Stk* stk) {
    int s = 0, e = (*stk).top;
    Node tmp;
    while(s < e) {
        tmp = (*stk).num[s];
        (*stk).num[s] = (*stk).num[e];
        (*stk).num[e] = tmp;
        ++s;
        --e;
    }
}

void calculate() {
    Stk stk;
		result = 0;
    resetStack(stk);
    rePolish(&stk);
    reverseStack(&stk);
    result = calc(&stk);
}

/*
int main() {
    char str[1000], buffer[1000];
    while (gets(str)) {
        calculate(str, buffer);
        printf("%s\n", buffer);
    }
}
*/


void display_string()
{
	unsigned char counter;
	char trailZero;
	float weight;
	LcdWriteCom( 0xC0 );
	LcdWriteCom( 0x06 );
	weight = 1e16;
	/*
	if ( result * 1e4 -((int)(result * 1e3 ))*10 >= 5)
	{
		result = ((int)(result*1e3)+1)/1e3;
	}
	*/
	result += 0.0005;
	if (result / weight >= 10) {
	    LcdWriteData('E');
	    LcdWriteData('r');
	    LcdWriteData('r');
	    LcdWriteData('o');
	    LcdWriteData('r');
	    return;
	}
	while((int)(result / weight) == 0) {
	    if (result == 0) {
	        break;
	    }
	    weight /= 10;
	}
	trailZero = 0;
	for( counter = 0; counter < 16 && trailZero < 4; counter++)
	{
	    LcdWriteData((int)( result / weight) + '0');
	    result = result - ((int)(result / weight)) * weight;
	    if (trailZero) {
	        ++trailZero;
	    }
	    if (((int)weight) == 1) {
	        LcdWriteData('.');
	        trailZero = 1;
	    }
	    weight /= 10;
		//LcdWriteData( Lcd_result_string[counter] );
	}
}


			
void main()
{
	unsigned char key;
	char counter = 0;
	LcdInit();
	while(1)
	{
		if ( get_key( &key ) == 1 )
		{
		 	if ( key == '=' )
			{
#ifdef __CALC__
				calculate();
				display_string();
				while( 1 )
				{
					if ( get_key( &key ) == 1)
					{
						LcdWriteCom( 0x01 );
						LcdWriteCom( 0x06 );
						if ( key != 'C' )
							LcdWriteData( key );
						break;
					}
				}
#endif
				//-------------- test extend rom ----
				
#ifdef __ROM_TEST__
				LcdWriteCom( 0xC0 );
				for(key = 0; key <=counter; key++)
				{
					LcdWriteData( RomRead( key ) );
				}
#endif
				
				//-------------- end

			}
			else	if ( key == 'C' )
			{
				LcdWriteCom( 0x01 );
				LcdWriteCom( 0x06 );
				counter = 0;
			}
			else
			{
				if ( counter == 16 )
				{
					LcdWriteCom( 0x07 ); 	
				}
				LcdWriteData( key );
				
				//-------------- test extend rom ----
				
#if __ROM_TEST__

				RomWrite(counter, key);
#endif
				
				//-------------- end
				counter++;
			}

			if ( counter == 40 )
			{
				LcdWriteCom( 0x06 );
				LcdWriteCom( 0x01 );
			}
		}	
	}
}


