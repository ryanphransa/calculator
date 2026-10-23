#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define GPIO_OUT_REG    0x3FF44004
#define GPIO_ENABLE_REG 0x3FF44020
#define GPIO_IN_REG     0x3FF4403C
#define SENS_SAR_DAC_CTRL1_REG 0x3FF48898
#define SENS_SAR_DAC_CTRL2_REG 0x3FF4889C
/*
 * Adjust frequency of CW with SENS_SAR_SW_FSTEP [15:0]
 * SENS_SW_FSTEP[16:0] Frequency step for CW generator
 * can be used to adjust the frequency.
 * freq = dig_clk_rtc_freq * SENS_SAR_SW_FSTEP / 65536
 * the frequency of dig_clk_rtc is typically 8MHz
 */
#define RTCIO_PAD_DAC1_REG 0x3FF48484
#define RTCIO_PAD_DAC2_REG 0x3FF48488

#define REG_READD(reg) (*(volatile uint32_t *)(reg))

/*
   This projects depends of turning off
   watchdog verify interval
*/

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
      case '+':
      case 17: // +
        printf("+ ");
        return true;
      case '-':
      case 5: // -
        printf("- ");
        return true;
      default:
        return false;
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

double expression(char *operator, double *operand) {
    float results[BUFFER_SIZE];
    float result = 0;
    size_t my_op = 0;

    size_t ignore_it = 1; // cosmetic index '1)'

    for (size_t i = 0; ; i++) {
      printf("socorro\n");
        if (pemdas(operator[i])/*+parentheses[i]*/ > pemdas(operator[my_op])/*+parentheses[my_op])*/) {
            my_op = i;
        }

        if (operator[i] == '\0') {
            if (operator[my_op] == '\0') {
              printf("op = /0 com i:%d\n", i);
              return operand[0];
            }

            // below has cosmetic-print code
            /*printf("\t%d) ", ignore_it++);
            for (size_t k = 0; operand[k] != '\0'; k++) {
                printf("%.2f ", operand[k]);
                if (isoperator(operator[k])) {
                    if (parentheses[k] > 0)
                        printf("%cp ", operator[k]);
                    else
                        printf("%c ", operator[k]);
                }
            } putchar('\n');*/

            printf("operand[my_op] = %f %c %f\n", operand[my_op], operator[my_op], operand[my_op+1]);
            operand[my_op] = operation(operand[my_op], operand[my_op+1], operator[my_op]);
            printf("Resultado medio: %f\n", operand[my_op]);

            delete_operand(operand, my_op+1);
            delete_operator(operator, my_op);
            delete_parentheses(my_op); // parentheses == operator, so we should delete the same element in both

            my_op = 0;
            i = 0;
        }
    }
}

void app_main(void)
{
  volatile uint32_t* gpio_out = (volatile uint32_t*)GPIO_OUT_REG;
  volatile uint32_t* gpio_enable = (volatile uint32_t*)GPIO_ENABLE_REG;
  volatile uint32_t* dac_freq = (volatile uint32_t*)SENS_SAR_DAC_CTRL1_REG;
  volatile uint32_t* dac_power_on = (volatile uint32_t*)RTCIO_PAD_DAC1_REG;
  volatile uint32_t* dac_cw = (volatile uint32_t*)SENS_SAR_DAC_CTRL2_REG;

  *dac_cw = 1 << 24;

  //*dac_power_on = (1 << 10) + (1 << 18);  // dac power on
  /* tristate now */
  *dac_power_on += 0 << 27; // RUE OFF
  *dac_power_on += 0 << 28; // RDE OFF
  *dac_power_on += 0 << 11; // FUN_IE OFF
  *dac_power_on += 0 << 12; // SLP_OE OFF
  *dac_power_on += 0 << 13; // SLP_IE OFF

  *dac_freq = (1 << 16); // SENS_SW_FSTEP Frequency step for CW generator;
                         // can be used to adjust the frequency.

  *dac_freq += (1 << 3); // define freq

  int button_gpio[31] = {0};
  button_gpio[4] = 1;
  button_gpio[16] = 1;
  button_gpio[17] = 1;
  button_gpio[5] = 1;
  button_gpio[18] = 1;

  int gpio[31] = {0};
  gpio[2] = 1;
  gpio[25] = 1;

  /* Enables LED pin as output */
  for (size_t i = 0; i <= 31; i++) {
    if (gpio[i] && !button_gpio[i]) {
      *gpio_enable += (1 << i);
    }
  }

  // math
  double operand[100] = {0};
  char operator[100] = {0};
  size_t j = 0, k = 0;
  double n = 0;
  char end = 0;

  while(1) {
    for (size_t i = 0; ; i++) {
      if (button_gpio[i] && ((REG_READD(GPIO_IN_REG)) >> i) & 0x01) {
        *gpio_out = *gpio_enable;
        *dac_power_on = (1 << 10) + (1 << 18);  // dac power on
        printf("index: %d\n", i);
        if (i == 4) {
          printf("1\n");
          n = n * 10 + 1;
        }
        else if (i == 16) {
          printf("1\n");
          n = n * 10 + 1;
        }
        else if (i == 17) {
          printf("+\n");
          if (n != 0)
              operand[j++] = n;
          n = 0;
          operator[k++] = '+';
        }
        else if (i == 5) {
          printf("-\n");
          if (n != 0)
              operand[j++] = n;
          n = 0;
          operator[k++] = '-';
        }
        else if (i == 18) {
          printf("end expression\n");
          operand[j++] = n;
          end = 1;
        }
        delay_ms(200);
        break;
      }
      else if (i > 31) {
        *gpio_out &= ~*gpio_enable;
        *dac_power_on = (0 << 10) + (0 << 18);  // dac power off
        break;
      }
    }

    if (end) {
      printf("quebrei\n");
      operand[j] = '\0';
      operator[k] = '\0';
      *gpio_out &= ~*gpio_enable;
      *dac_power_on = (0 << 10) + (0 << 18);  // dac power off

      for (size_t z = 0; z < j; z++) {
        printf("%f ", operand[z]);
        printf("%c ", operator[z]);
      }

      double result = expression(operator, operand);
      printf("Result: %f\n", result);

      j = 0, k = 0;
      n = 0;
      end = 0;
    }
  }
}
