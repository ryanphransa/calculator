#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define GPIO_OUT_REG    0x3FF44004
#define GPIO_OUT_ENABLE 0x3FF44020
#define GPIO_IN_REG     0x3FF4403C

#define REG_READ(reg) (*(volatile uint32_t *)(reg))

enum {
    BUFFER_SIZE = 100
};

double parentheses[BUFFER_SIZE] = {0};
char variables[BUFFER_SIZE] = {0};
bool variable_on = false;

int pemdas(char operator) {
    switch (operator) {
        case '+': return 10;
        case '-': return 10;
        case '*': return 20;
        case '/': return 20;
        case '"': return 30;
        default:
            //printf("PEMDAS error: Not found a valid operator\n");
            return 0;
    }
}

double operation(double op1, double op2, char operator) {
    switch (operator) {
        case '+': return op1 + op2;
        case '-': return op1 - op2;
        case '*': return op1 * op2;
        case '/': return op1 / op2;
        case '"':
            for (; op2 > 1; op2--) {
                op1 *= op1;
            }
            return op1;
        default:
            printf("Operation error: Not found a valid operator.\n");
            return 0;
    }
}

bool isoperator(uint32_t button_pressed_gpio) {
    switch(button_pressed_gpio) {
        case 17: // +
          printf("+ ");
          return true;
        case 5: // -
          printf("- ");
          return true;
        case 3: // *
        case 4: // /
        case 2: // "
            return true;
        default:
            return false;
    }
}

void delay_ms(unsigned int ms)
{
  while(ms--) {
    for(int count = 0; count < 24000; count++) {
      /* Assembly code for giving a free cpu cycle
        to create a delay, nop -> NO OPERATION */
      __asm__ volatile ("nop");
    }
  }
}

void fakegetline(char *operator, double *operand) {
  volatile uint32_t* gpio_out = (volatile uint32_t*)GPIO_OUT_REG;
  volatile uint32_t* gpio_enable = (volatile uint32_t*)GPIO_OUT_ENABLE;

  int button_gpio[31] = {0};
  button_gpio[4] = 1;
  button_gpio[16] = 1;
  button_gpio[17] = 1;
  button_gpio[5] = 1;
  button_gpio[18] = 1;

  int gpio[31] = {0};
  gpio[2] = 1;

  double button_pressed_gpio = 0;

  /* indexes */
  size_t i = 0, j = 0;
  double n = 0, f = 10;
  bool float_point = false;

  bool parentheses_on = false;
  int parentheses_nest = 0;

  /* Enables LED pin as output */
  for (size_t i = 0; i <= 31; i++) {
    if (gpio[i] && !button_gpio[i]) {
      *gpio_enable += (1 << i);
    }
  }

  while(1) { // while what?
    for (size_t i = 0; ; i++) {
      if (button_gpio[i] && ((REG_READ(GPIO_IN_REG)) >> i) & 0x01) {
        *gpio_out = *gpio_enable;
        button_pressed_gpio = (double)i;
        break;
      }
      else if (i > 31) {
        *gpio_out &= ~*gpio_enable;
        break;
      }
    }
    /*
     * This algorithm depends of the
     * button_pressed_gpio being exactly
     * the same number as the number he
     * represents
     * ex.: GPIO 1 = (double)1
     */
    if (button_pressed_gpio == 4 ||
        button_pressed_gpio == 16) {
        printf("%f ", button_pressed_gpio);
        if (!float_point)
            n = n * 10 + (double)button_pressed_gpio;
        else {
            n = n + (double)button_pressed_gpio / f;
            f *= 10;
        }
    } else if (button_pressed_gpio == '.') {
        float_point = true;
    } else if (button_pressed_gpio == '(') {
        parentheses_on = true;
        parentheses_nest += 100;
    } else if (button_pressed_gpio == ')') {
        parentheses_nest -= 100;
        if (parentheses_nest < 100)
            parentheses_on = false;
    /*
     * Should reconfig isoperator()
     */
    } else if (isoperator(button_pressed_gpio)) {
        if (parentheses_on)
            parentheses[j] = parentheses_nest;
        if (n != 0)
            operand[i++] = n;
        //operator[j++] = c;

        float_point = false;
        n = 0, f = 10;
    } else if (isalpha(0)) { // ignoring this part
        // variable
        variable_on = true;
        //variables[i++] = c; // use the same index as operand
    } else if (button_pressed_gpio == 18) { // END button
        operand[i++] = n;
        break;
    }
    operand[i] = operator[i] = '\0';
    delay_ms(1000); // timer is wrong, correct it
  }
}

void delete_operand(double *operand, size_t index) {
    for (; operand[index] != '\0'; index++) {
        operand[index] = operand[index+1];
    }
}

void delete_operator(char *operator, size_t index) {
    for (; operator[index] != '\0'; index++) {
        operator[index] = operator[index+1];
    }
}

void delete_parentheses(size_t index) {
    for (; parentheses[index] != '\0'; index++) {
        parentheses[index] = parentheses[index+1];
    }
}

double equation(char *operator, double *operand, char *variables) {
    // find a b and c
    // quadratic example
    // a*x"2+b*x+c
    // 12*x"2-2*x+2 = 0
    double a = 0, b = 0, c = 0, x, x1, x2;
    for (size_t i = 0; ; i++) {
        if (variables[i] != 0 && a == 0)
            a = operand[i-1];
        else if (variables[i] != 0 && b == 0) {
            if (operator[i-2] == '-')
                b = operand[i-1]*-1;
            else
                b = operand[i-1];
            if (operator[i] == '-')
                c = operand[i+1]*-1;
            else
                c = operand[i+1];
            break;
        }
    }

    for (size_t i = 0; i < 10; i++) {
        printf("index: %d\n", i);
        printf("\toperand -> %.2f\n", operand[i]);
        printf("\toperator -> %c\n", operator[i]);
        printf("\tvariables -> %c\n", variables[i]);
        putchar('\n');
    } putchar('\n');


    printf("a = %f, b = %f, c = %f\n", a, b, c);

    double delta = b*b - 4*a*c;
    if (delta < 0)
        printf("Δ = %.2f, no real roots\n", delta);
    else if (delta == 0) {
        x = -b / 2*a;
        printf("Δ = %.2f\n x₁= x₂= %.2f\n", delta, x);
    }
    else {
        x1 = (-b + sqrt(delta)) / (2*a);
        x2 = (-b - sqrt(delta)) / (2*a);
        printf("Δ = %.2f\nx₁= %.2f\nx₂= %.2f\n", delta, x1, x2);
    }

    return 0;
}

double expression(char *operator, double *operand) {
    float results[BUFFER_SIZE];
    float result = 0;
    size_t my_op = 0;

    size_t ignore_it = 1; // cosmetic index '1)'

    for (size_t i = 0; ; i++) {
        if (pemdas(operator[i])+parentheses[i] > pemdas(operator[my_op])+parentheses[my_op]) {
            my_op = i;
        }

        if (operator[i] == '\0') {
            if (operator[my_op] == '\0')
                return operand[0];

            // below has cosmetic-print code
            printf("\t%d) ", ignore_it++);
            for (size_t k = 0; operand[k] != '\0'; k++) {
                printf("%.2f ", operand[k]);
                if (isoperator(operator[k])) {
                    if (parentheses[k] > 0)
                        printf("%cp ", operator[k]);
                    else
                        printf("%c ", operator[k]);
                }
            } putchar('\n');

            operand[my_op] = operation(operand[my_op], operand[my_op+1], operator[my_op]);

            delete_operand(operand, my_op+1);
            delete_operator(operator, my_op);
            delete_parentheses(my_op); // parentheses == operator, so we should delete the same element in both

            my_op = 0;
            i = 0;
        }
    }
}

int app_main() {
    char operator[BUFFER_SIZE];
    double operand[BUFFER_SIZE];
    double result = 0;

    while (1) {
        printf("Expression: ");
        fakegetline(operator, operand);
        
        if (!variable_on) {
            result = expression(operator, operand);
            printf("Result: %f", result);
        }
        else
            result = equation(operator, operand, variables);
    }
    return 0;
}
