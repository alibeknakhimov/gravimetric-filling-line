/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

//========================================================
// НАСТРОЙКА СТАНКА
// Укажите, сколько флаконов нужно накопить перед началом розлива
#define TARGET_BOTTLES 5

// Мягкость розлива против всплесков на этикетку (крутить вручную):
//   0 : агрессивнее / быстрее, сопло чуть выше, дольше быстрый насос
//   5 : норма (старт)
//  10: мягче: сопло ближе к зеркалу, раньше замедление насоса
#define SPLASH_SOFTNESS 5
/* Скорости насоса (период импульсов в тиках TIM6-счётчика rozliv_count):
 *   rozliv=1: быстро: было >1 (каждые 2 тика); >0 = каждый тик (~×2)
 *   rozliv=3: мягкий старт (между быстрым и СТАРЫМ медленным >10)
 *   rozliv=2: финиш, ~в 1.75 раза медленнее старого (>10 → >18) */
#define ROZLIV_SOFT_PERIOD  5
#define ROZLIV_SLOW_PERIOD  18
#define ROZLIV_FAST_PERIOD  1   /* было 0 (каждый тик = опасно у горла); 1 = спокойнее */

/*
 * Пороги налиты/отлажены на режиме 93 г (REF). Для 50 г те же доли от дозы :
 * поведение как на 93, только сжатое по массе.
 */
#define REF_DOSE_DG              930  /* ves=93 → дециграммы */
#define SOFT_START_DG_93         50   /* ~5 г мягкий насос */
#define FILL_ARM_DG_93           100  /* ~10 г: старт флага у дна */
#define LIFT_FOLLOW_START_DG_93  310  /* ~31 г: начало подъёма сопла */
#define DRIP_TAIL_DG_93          20   /* запас до капельного стопа */
#define STOP_TAIL_DG_93          35   /* жёсткий стоп до цели */

/*
 * Высота сопла (как на учебном стенде, НЕ через «offset»):
 * 1) пока масса мала: сопло СИДИТ у низа (spusk), пока зеркало не подойдёт;
 * 2) затем поднимается пропорционально массе, но только на долю хода.
 * NOZZLE_RISE_PERMILLE: основной зазор (от 350): 635 ≈ ещё чуть больше после 580.
 * К концу дозы: доподъём (NOZZLE_END_*).
 */
#define NOZZLE_RISE_PERMILLE      635
#define NOZZLE_END_START_PERMILLE 850  /* с ~85% дозы: доподъём */
#define NOZZLE_END_RISE_PERMILLE  980  /* финиш: почти полный ход вверх */
//========================================================

int flag1=1;
int updown1=1;
int d1=0;
int rozliv1=0;
int rozliv_count1=0;
int flag_rozliv1=0;
int a1=0;
//----------------------------------------------------------------------------------------
int flag2=1;
int updown2=1;
int d2=0;
int rozliv2=0;
int rozliv_count2=0;
int flag_rozliv2=0;
int a2=0;
//----------------------------------------------------------------------------------------
int flag3=1;
int updown3=1;
int d3=0;
int rozliv3=0;
int rozliv_count3=0;
int flag_rozliv3=0;
int a3=0;
//----------------------------------------------------------------------------------------
int flag4=1;
int updown4=1;
int d4=0;
int rozliv4=0;
int rozliv_count4=0;
int flag_rozliv4=0;
int a4=0;
//----------------------------------------------------------------------------------------
int flag5=1;
int updown5=1;
int d5=0;
int rozliv5=0;
int rozliv_count5=0;
int flag_rozliv5=0;
int a5=0;
//----------------------------------------------------------------------------------------
volatile int stepper=0;
volatile int counter=0;
int step=0;
int step2=0;
int stepspeed=7;
volatile int metka=0;
int met=0;
int count_metka=0;
int flag_metka=0;
int metka2=0;
volatile int flag_one=0;
volatile int carousel_steps_left=0;
volatile int fill_pos_ready=0;

int rrz1=0;
int rrz2=0;
int rrz3=0;
int rrz4=0;
int rrz5=0;

int flag_vxod=0;
int flag_vyxod=0;
int flag_krutka=0;
int krutka=0;
int ms_vyxod=0;
int vyxod=0;
int vyxod_in=0;
int counter_vyxod=0;

int butylka=0;
int rezhim=0;
int dimona_knopka=1;
int flag50=0;
int spusk=0;
int spusk2=0;
int vxodsec=0;
int r1=0;
int r2=0;
int r3=0;
int r4=0;
int r5=0;

int zakr1=0;
int zakr2=0;
int zakr3=0;
int zakr4=0;
int zakr5=0;

int rs1=0;
int rs2=0;
int rs3=0;
int rs4=0;
int rs5=0;


volatile int flagi=0;
char str[50];
int av[6];
int butyl[6];
int vesy[6];
int av2[6];
int kui=0;
int ss[6]={3,3,3,3,3,3};
int ss2[6]={6500,6500,6500,6500,6500,6500};
int count=0;
int count2=0;
int flag_count[6]={0,0,0,0,0,0};
int a=0;
int av3[6];
int verx_count1=0;
int verx_count2=0;
int verx_count3=0;
int verx_count4=0;
int verx_count5=0;
int ves=50;
int alibek_metka = 0;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t modbus_ids[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t current_sensor_index = 0;
uint8_t scale_updated[5] = {0, 0, 0, 0, 0};
/* Время последнего успешного Modbus-ответа по каждой колонне (для стопа при «слепых» весах) */
uint32_t scale_last_ok_ms[5] = {0, 0, 0, 0, 0};
uint32_t fill_start_ms = 0; /* момент входа в rezhim==3 */
uint8_t fill_dose_hit[5] = {0, 0, 0, 0, 0}; /* доза однажды достигнута: насос больше не включать */

typedef enum { MODBUS_IDLE, MODBUS_WAIT_RESPONSE } modbus_state_t;
modbus_state_t modbus_state = MODBUS_IDLE;

uint8_t rs485_rx_buffer[9];
uint8_t rs485_rx_index = 0;
uint8_t rs485_response_complete = 0;
uint8_t rs485_rx_byte = 0;

uint32_t rs485_last_poll_time = 0;
uint32_t rs485_response_start_time = 0;
uint32_t last_tare_time = 0;
uint32_t tare_button_press_start = 0;
uint8_t tare_button_done = 0; /* один сброс на одно удержание (пока не отпустят) */

#define POLL_PERIOD_MS 35
#define RESPONSE_TIMEOUT_MS 25
/* Налив только при свежих весах; иначе насос стоп (цикл не блокируем) */
#define FILL_SCALE_STALE_MS  400   /* нет ответа дольше → пауза импульсов этой колонны */
#define FILL_MAX_MS         45000 /* аварийный стоп всего налива */

/* USB CDC: снимок в usb_mass_request_send(); отправка фоном из TIM6 (~5 мс),
 * даже во время HAL_Delay / tare. Сам TX: в USB IRQ. DMA не нужен.
 * Формат: m1;m2;m3;m4;m5;dose;st\r\n
 *   dose: 50 или 93
 *   st  : 0 обычный (брутто), 1 налив (нетто), 2 факт налива (нетто, удержание) */
extern USBD_HandleTypeDef hUsbDeviceFS;
static int usb_mass_snap[5];
static int usb_snap_dose;
static int usb_snap_st;
static int usb_fill_latch[5];
static volatile uint8_t usb_fill_hold; /* сколько успешных TX со st=2 */
static volatile uint8_t usb_mass_pending;
static uint8_t usb_mass_tx_buf[64];
/* TIM6 ~5 кГц → каждые 25 тиков ≈ 5 мс попытка отправки */
#define USB_MASS_POLL_DIV 25
static uint16_t usb_mass_poll_div;

static void usb_mass_request_send(void)
{
  int i;

  if (usb_fill_hold > 0) {
    for (i = 0; i < 5; i++) {
      usb_mass_snap[i] = usb_fill_latch[i];
    }
    usb_snap_st = 2;
  } else if (rezhim == 3) {
    for (i = 0; i < 5; i++) {
      usb_mass_snap[i] = av[i]; /* при аппаратной таре = нетто налива */
    }
    usb_snap_st = 1;
  } else {
    for (i = 0; i < 5; i++) {
      usb_mass_snap[i] = av[i];
    }
    usb_snap_st = 0;
  }

  usb_snap_dose = (flag50 == 0) ? 50 : 93;
  usb_mass_pending = 1;
}

static uint8_t *usb_append_int(uint8_t *p, int v)
{
  char dig[12];
  unsigned int u;
  int n = 0;

  if (v < 0) {
    *p++ = '-';
    u = (unsigned int)(-(v + 1)) + 1u;
  } else {
    u = (unsigned int)v;
  }

  do {
    dig[n++] = (char)('0' + (u % 10u));
    u /= 10u;
  } while (u != 0u);

  while (n > 0) {
    *p++ = (uint8_t)dig[--n];
  }
  return p;
}

/* Фоновая отправка: вызов из TIM6 (не из while). Если CDC занят: выход сразу. */
static void usb_mass_poll(void)
{
  uint8_t *p;
  int i;
  USBD_CDC_HandleTypeDef *hcdc;

  if (usb_mass_pending == 0) {
    return;
  }
  if (hUsbDeviceFS.pClassData == NULL) {
    return;
  }
  hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0) {
    return;
  }

  p = usb_mass_tx_buf;
  for (i = 0; i < 5; i++) {
    if (i != 0) {
      *p++ = ';';
    }
    p = usb_append_int(p, usb_mass_snap[i]);
  }
  *p++ = ';';
  p = usb_append_int(p, usb_snap_dose);
  *p++ = ';';
  p = usb_append_int(p, usb_snap_st);
  *p++ = '\r';
  *p++ = '\n';

  if (CDC_Transmit_FS(usb_mass_tx_buf, (uint16_t)(p - usb_mass_tx_buf)) == USBD_OK) {
    usb_mass_pending = 0;
    if (usb_snap_st == 2 && usb_fill_hold > 0) {
      usb_fill_hold--;
    }
  }
}

uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t buffer_length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < buffer_length; i++) {
    crc ^= buffer[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void modbus_send_request(uint8_t *request, uint8_t len) {
  uint16_t crc = Modbus_CRC16(request, len - 2);
  request[len - 2] = crc & 0xFF;
  request[len - 1] = (crc >> 8) & 0xFF;
  HAL_UART_Transmit(&huart2, request, len, 100);
}

void calculate_speeds(void) {
  for (int i = 0; i < 5; i++) {
    av3[i] = av[i] - av2[i];
    if (av3[i] < 5) { ss[i] = 11; }
    if (av3[i] < 10 && av3[i] >= 5) { ss[i] = 6; }
    if (av3[i] >= 10 && av3[i] < 20) { ss[i] = 5; }
    if (av3[i] >= 20) { ss[i] = 4; }
    av2[i] = av[i];
  }
}

void modbus_process(void) {
  uint32_t current_time = HAL_GetTick();
  uint8_t request[8] = {0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};

  switch (modbus_state) {
  case MODBUS_IDLE:
    if ((current_time - rs485_last_poll_time) >= POLL_PERIOD_MS) {
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      __HAL_UART_CLEAR_NEFLAG(&huart2);
      __HAL_UART_CLEAR_FEFLAG(&huart2);
      HAL_UART_AbortReceive(&huart2);
      
      request[0] = modbus_ids[current_sensor_index];
      
      rs485_rx_index = 0;
      rs485_response_complete = 0;
      modbus_state = MODBUS_WAIT_RESPONSE;
      rs485_last_poll_time = current_time;
      rs485_response_start_time = current_time;
      
      HAL_UART_Receive_IT(&huart2, &rs485_rx_byte, 1);
      modbus_send_request(request, 8);
    }
    break;

  case MODBUS_WAIT_RESPONSE:
    if (rs485_response_complete) {
      uint16_t calc_crc = Modbus_CRC16(rs485_rx_buffer, 7);
      uint16_t rx_crc = rs485_rx_buffer[7] | (rs485_rx_buffer[8] << 8);
      
      if (calc_crc == rx_crc && rs485_rx_buffer[0] == modbus_ids[current_sensor_index] && rs485_rx_buffer[1] == 0x03) {
        /* QLST manual §5.2: Real-time weight = 32-bit signed, regs 0..1.
         * Addr0 = low word, addr1 = high word.
         * Пример мануала: 04 D2 | 00 00 → 1234 */
        uint16_t lo = ((uint16_t)rs485_rx_buffer[3] << 8) | (uint16_t)rs485_rx_buffer[4];
        uint16_t hi = ((uint16_t)rs485_rx_buffer[5] << 8) | (uint16_t)rs485_rx_buffer[6];
        int32_t mass32 = (int32_t)(((uint32_t)hi << 16) | (uint32_t)lo);
        av[current_sensor_index] = (int)mass32; /* единицы модуля (обычно 0.1 г) */
        scale_updated[current_sensor_index] = 1;
        scale_last_ok_ms[current_sensor_index] = HAL_GetTick();
      }
      
      current_sensor_index++;
      if (current_sensor_index >= 5) {
        current_sensor_index = 0;
        calculate_speeds(); // Расчет скоростей после обновления всех датчиков
        usb_mass_request_send();
      }
      
      modbus_state = MODBUS_IDLE;
      rs485_response_complete = 0;
    } else if ((current_time - rs485_response_start_time) >= RESPONSE_TIMEOUT_MS) {
      current_sensor_index++;
      if (current_sensor_index >= 5) {
        current_sensor_index = 0;
        calculate_speeds();
        usb_mass_request_send();
      }
      modbus_state = MODBUS_IDLE;
      HAL_UART_AbortReceive(&huart2);
    }
    break;
  }
}

/*
 * Алгоритм розлива: мягкий старт → подъём сопла → ступени → стоп.
 * Жёстко: стоп по дозе и пауза без свежих весов: ДО любых early return.
 */
static void fill_channel_force_stop(int *flag_rozliv, int *flag_count,
                                    int *updown, int *rozliv, int *zakr)
{
  *rozliv = 0;
  *updown = 1;
  *flag_count = 0;
  *flag_rozliv = 0;
  *zakr = 0;
}

static void update_fill_channel(int vesy_i, int *flag_rozliv, int *flag_count,
                                int *updown, int *rozliv, int *zakr, int d,
                                int *ss2_i, int ch)
{
  int dose_dg;
  int fill_range;
  int rise_range;
  int m_prog;
  int m_range;
  int target_d;
  int t_fast;
  int t_mid;
  int t_drip;
  int soft = SPLASH_SOFTNESS;
  int soft_start_dg;
  int fill_arm_dg;
  int lift_start_dg;
  int drip_tail_dg;
  int stop_tail_dg;
  uint32_t now;
  uint32_t age;

  if (soft < 0) {
    soft = 0;
  }
  if (soft > 10) {
    soft = 10;
  }

  dose_dg = ves * 10;
  if (dose_dg < 1) {
    dose_dg = 1;
  }

  soft_start_dg = (dose_dg * SOFT_START_DG_93) / REF_DOSE_DG;
  fill_arm_dg = (dose_dg * FILL_ARM_DG_93) / REF_DOSE_DG;
  lift_start_dg = (dose_dg * LIFT_FOLLOW_START_DG_93) / REF_DOSE_DG;
  drip_tail_dg = (dose_dg * DRIP_TAIL_DG_93) / REF_DOSE_DG;
  stop_tail_dg = (dose_dg * STOP_TAIL_DG_93) / REF_DOSE_DG;
  if (soft_start_dg < 1) {
    soft_start_dg = 1;
  }
  if (fill_arm_dg < 1) {
    fill_arm_dg = 1;
  }
  if (lift_start_dg < 1) {
    lift_start_dg = 1;
  }
  if (drip_tail_dg < 1) {
    drip_tail_dg = 1;
  }
  if (stop_tail_dg < 1) {
    stop_tail_dg = 1;
  }
  if (lift_start_dg > (dose_dg - stop_tail_dg - 10)) {
    lift_start_dg = dose_dg / 3;
  }

  /* --- Страховка: работает даже если flag_count/rozliv «залипли» --- */
  if (*flag_rozliv == 1 || *rozliv != 0) {
    now = HAL_GetTick();
    if (fill_start_ms != 0 && (now - fill_start_ms) > FILL_MAX_MS) {
      fill_channel_force_stop(flag_rozliv, flag_count, updown, rozliv, zakr);
      return;
    }
    if ((ch >= 0 && ch < 5 && fill_dose_hit[ch]) ||
        vesy_i >= dose_dg || vesy_i > (dose_dg - stop_tail_dg)) {
      if (ch >= 0 && ch < 5) {
        fill_dose_hit[ch] = 1;
      }
      fill_channel_force_stop(flag_rozliv, flag_count, updown, rozliv, zakr);
      return;
    }
    if (ch >= 0 && ch < 5) {
      age = now - scale_last_ok_ms[ch];
      if (scale_last_ok_ms[ch] == 0 || age > FILL_SCALE_STALE_MS) {
        *rozliv = 0; /* эта колонна молчит: импульсы стоп; остальные не трогаем */
        return;
      }
    }
  }

  if (*flag_rozliv != 1) {
    return;
  }

  fill_range = spusk - spusk2;
  if (fill_range < 1) {
    fill_range = 1;
  }

  /* Старт насоса у дна: мягкий режим; НЕ ставить ss2=spusk2 */
  if (vesy_i < fill_arm_dg && *flag_count == 0 && d > (spusk - 10)) {
    *flag_count = 1;
    *rozliv = 3;
    *ss2_i = spusk;
    *updown = 3;
  }

  if (*flag_count == 0) {
    return;
  }

  /* --- Высота сопла --- */
  if (vesy_i <= lift_start_dg) {
    *updown = 3;
    *ss2_i = spusk;
  } else {
    m_range = dose_dg - lift_start_dg;
    if (m_range < 1) {
      m_range = 1;
    }
    m_prog = vesy_i - lift_start_dg;
    if (m_prog < 0) {
      m_prog = 0;
    }
    if (m_prog > m_range) {
      m_prog = m_range;
    }

    rise_range = (fill_range * NOZZLE_RISE_PERMILLE) / 1000;
    if (rise_range < 1) {
      rise_range = 1;
    }
    if (rise_range > fill_range) {
      rise_range = fill_range;
    }

    target_d = spusk - (m_prog * rise_range) / m_range;

    {
      int end_start = (dose_dg * NOZZLE_END_START_PERMILLE) / 1000;
      if (vesy_i > end_start) {
        int end_target = spusk - (fill_range * NOZZLE_END_RISE_PERMILLE) / 1000;
        int ep;
        int er;
        if (end_target < spusk2) {
          end_target = spusk2;
        }
        ep = vesy_i - end_start;
        er = dose_dg - end_start;
        if (er < 1) {
          er = 1;
        }
        if (ep > er) {
          ep = er;
        }
        if (end_target < target_d) {
          target_d = target_d - ((target_d - end_target) * ep) / er;
        }
      }
    }

    if (target_d < spusk2) {
      target_d = spusk2;
    }
    if (target_d > (spusk - 10)) {
      target_d = spusk - 10;
    }
    *ss2_i = target_d;
    *updown = 4;
  }

  /* --- Профиль насоса ---
   * Раньше быстрый почти до 90% → у горла ещё полный ход.
   * Теперь быстрый примерно до 72% (soft=5), дальше rozliv=2. */
  t_fast = (dose_dg * (820 - soft * 20)) / 1000;
  t_mid = (dose_dg * (900 - soft * 10)) / 1000;
  if (t_fast < (dose_dg * 65) / 100) {
    t_fast = (dose_dg * 65) / 100;
  }
  if (t_fast > (dose_dg * 80) / 100) {
    t_fast = (dose_dg * 80) / 100;
  }
  if (t_mid <= t_fast) {
    t_mid = t_fast + 1;
  }
  t_drip = dose_dg - drip_tail_dg;
  if (t_drip <= t_mid) {
    t_drip = t_mid + 1;
  }

  if (vesy_i < soft_start_dg) {
    *rozliv = 3;
  } else if (vesy_i < t_fast) {
    *rozliv = 1;
  } else if (vesy_i < t_drip) {
    *rozliv = 2;
  } else {
    if (ch >= 0 && ch < 5) {
      fill_dose_hit[ch] = 1;
    }
    fill_channel_force_stop(flag_rozliv, flag_count, updown, rozliv, zakr);
  }

  if (vesy_i > (dose_dg - stop_tail_dg)) {
    if (ch >= 0 && ch < 5) {
      fill_dose_hit[ch] = 1;
    }
    fill_channel_force_stop(flag_rozliv, flag_count, updown, rozliv, zakr);
  }
}

void set_tare_active_scales(void) {
  uint8_t request[8] = {0x00, 0x06, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00};
  uint8_t active[5] = {flag_rozliv1, flag_rozliv2, flag_rozliv3, flag_rozliv4, flag_rozliv5};
  uint8_t dummy_rx[8];
  
  HAL_UART_AbortReceive(&huart2);
  modbus_state = MODBUS_IDLE; // Reset modbus state to allow clean communication
  
  for (int i = 0; i < 5; i++) {
    if (active[i] == 1) {
      request[0] = modbus_ids[i];
      for (int retry = 0; retry < 3; retry++) {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        volatile uint32_t tmpreg = huart2.Instance->DR; // Flush HW RX Data Register
        (void)tmpreg;
        for(int k=0; k<8; k++) dummy_rx[k] = 0;
        
        while (huart2.gState != HAL_UART_STATE_READY) {}
        
        modbus_send_request(request, 8);
        if (HAL_UART_Receive(&huart2, dummy_rx, 8, 120) == HAL_OK) {
          if (dummy_rx[0] == modbus_ids[i] && dummy_rx[1] == 0x06) {
            break; // Success! Response matches request.
          }
        }
        HAL_Delay(20); // Delay before retry to let bus settle
      }
      HAL_Delay(30); // Delay before next scale
    }
  }
}

void cancel_tare_all_scales(void) {
  uint8_t request[8] = {0x00, 0x06, 0x00, 0x11, 0x00, 0x02, 0x00, 0x00};
  uint8_t dummy_rx[8];
  
  HAL_UART_AbortReceive(&huart2);
  modbus_state = MODBUS_IDLE; // Reset modbus state
  
  for (int i = 0; i < 5; i++) {
    request[0] = modbus_ids[i];
    for (int retry = 0; retry < 3; retry++) {
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      volatile uint32_t tmpreg = huart2.Instance->DR;
      (void)tmpreg;
      for(int k=0; k<8; k++) dummy_rx[k] = 0;
      
      while (huart2.gState != HAL_UART_STATE_READY) {}
      
      modbus_send_request(request, 8);
      if (HAL_UART_Receive(&huart2, dummy_rx, 8, 120) == HAL_OK) {
        if (dummy_rx[0] == modbus_ids[i] && dummy_rx[1] == 0x06) {
          break; // Success! Response matches request.
        }
      }
      HAL_Delay(20); // Delay before retry
    }
    HAL_Delay(30); // Delay before next scale
  }
}

void tare_all_scales(void) {
  /* Было опасно: 0x12=1 = ZERO CALIBRATION (портит калибровку в EEPROM).
   * Нужно оператору: обнулить показания (peeling), как в мануале QLST:
   *   01 06 00 11 00 01 : write 1 to reg 17
  uint8_t request[8] = {0x00, 0x06, 0x00, 0x12, 0x00, 0x01, 0x00, 0x00};
   */
  uint8_t request[8] = {0x00, 0x06, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00};
  uint8_t dummy_rx[8];
  
  HAL_UART_AbortReceive(&huart2);
  modbus_state = MODBUS_IDLE; // Reset modbus state
  
  for (int i = 0; i < 5; i++) {
    request[0] = modbus_ids[i];
    for (int retry = 0; retry < 3; retry++) {
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      volatile uint32_t tmpreg = huart2.Instance->DR;
      (void)tmpreg;
      for(int k=0; k<8; k++) dummy_rx[k] = 0;
      
      while (huart2.gState != HAL_UART_STATE_READY) {}
      
      modbus_send_request(request, 8);
      if (HAL_UART_Receive(&huart2, dummy_rx, 8, 120) == HAL_OK) {
        if (dummy_rx[0] == modbus_ids[i] && dummy_rx[1] == 0x06) {
          break; // Success! Response matches request.
        }
      }
      HAL_Delay(20); // Delay before retry
    }
    HAL_Delay(30); // Delay before next scale
  }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void carousel_step_pulse(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
}

/* Drum only. TIM7 priority 0, so nozzle and pump work in TIM6 cannot stretch a step. */
void IRQ_htim7(void)
{
	counter++;

	if (stepper == 0)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
		step2 = 0;
		return;
	}

	if (stepper == 1)
	{
		carousel_step_pulse();
		return;
	}

	if (counter <= stepspeed)
	{
		return;
	}
	counter = 0;

	if (stepper == 2)
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 3 && metka < step)
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 4 && metka != 640 && flagi > 7000)
	{
		if (flag_one > 0) metka++;
		carousel_step_pulse();
	}
	else if (stepper == 5 && metka != 1280)
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 6 && metka != 1920)
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 7 && metka != (640 * 4))
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 8 && metka != (640 * 5))
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 9 && metka != (640 * 6))
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 10 && metka != (640 * 7))
	{
		metka++;
		carousel_step_pulse();
	}
	else if (stepper == 55 && carousel_steps_left > 0)
	{
		metka++;
		if (metka > 6399) metka = 0;
		carousel_steps_left--;
		if (carousel_steps_left == 0)
		{
			stepper = 0;
			if (butylka == TARGET_BOTTLES) fill_pos_ready = 1;
		}
		carousel_step_pulse();
	}
}

void IRQ_htim6(void)
{
	flag1=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_15);
	flag2=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
	flag4=HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_5);
	flag5=HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3);
	flag3=HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_1);
	flag_metka=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_14);
	flag_vxod=HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4);
	flag_vyxod=HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_6);
	dimona_knopka=HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4);
	flag50=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_1);
	if(flag1==0||flag2==0||flag3==0||flag4==0||flag5==0) flagi=0;
	if(flag_vxod==1) vxodsec++;
	else vxodsec=0;
		
	if(zakr1==1) HAL_GPIO_WritePin(GPIOD,GPIO_PIN_0,GPIO_PIN_SET);
	if(zakr2==1)HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_SET);
	if(zakr3==1)HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
	if(zakr4==1)HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET); /* был PC9=закр5; PA9 убран (USB VBUS) */
	if(zakr1==0&&flag1==1)HAL_GPIO_WritePin(GPIOD,GPIO_PIN_0,GPIO_PIN_RESET);
	if(zakr2==0&&flag2==1)HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET);
	if(zakr3==0&&flag3==1)HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
	if(zakr4==0&&flag4==1)HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
	
	
	if(flag_metka==1)
	{
		if(met==0)
		{
			flag_one=1;
			met=1;
			metka2=metka;
			metka=0;
		}
	}

	
		if(flag_metka==0) met=0;
	
	
	
	rozliv_count1++;
	rozliv_count2++;
	rozliv_count3++;
	rozliv_count4++;
	rozliv_count5++;
	verx_count1++;
	verx_count2++;
	verx_count3++;
	verx_count4++;
	verx_count5++;
	counter_vyxod++;
if(r1>2000)rs1=0; /* турбо каждый тик отключено: у горла заливало */
if(r2>2000)rs2=0;
if(r3>2000)rs3=0;
if(r4>2000)rs4=0;
if(r5>2000)rs5=0;
//----------------------------------------------------------------------------------------
if(flag50==0)
{
	spusk=6500;
	spusk2=3300;
	ves=50;
}

if(flag50==1)
{
	spusk=6000;
	spusk2=2300;
	ves=93;
}
/* Жёсткий стоп насоса в IRQ: доза → навсегда; весы молчат → пауза */
if (rezhim == 3) {
	int dose_lim = ves * 10;
	int stop_m = (dose_lim * STOP_TAIL_DG_93) / REF_DOSE_DG;
	uint32_t tnow = HAL_GetTick();
	if (stop_m < 1) stop_m = 1;
	dose_lim = dose_lim - stop_m;
	if (fill_dose_hit[0] || (rozliv1 != 0 && av[0] >= dose_lim)) { fill_dose_hit[0] = 1; rozliv1 = 0; }
	else if (rozliv1 != 0 && (scale_last_ok_ms[0] == 0 || (tnow - scale_last_ok_ms[0]) > FILL_SCALE_STALE_MS)) { rozliv1 = 0; }
	if (fill_dose_hit[1] || (rozliv2 != 0 && av[1] >= dose_lim)) { fill_dose_hit[1] = 1; rozliv2 = 0; }
	else if (rozliv2 != 0 && (scale_last_ok_ms[1] == 0 || (tnow - scale_last_ok_ms[1]) > FILL_SCALE_STALE_MS)) { rozliv2 = 0; }
	if (fill_dose_hit[2] || (rozliv3 != 0 && av[2] >= dose_lim)) { fill_dose_hit[2] = 1; rozliv3 = 0; }
	else if (rozliv3 != 0 && (scale_last_ok_ms[2] == 0 || (tnow - scale_last_ok_ms[2]) > FILL_SCALE_STALE_MS)) { rozliv3 = 0; }
	if (fill_dose_hit[3] || (rozliv4 != 0 && av[3] >= dose_lim)) { fill_dose_hit[3] = 1; rozliv4 = 0; }
	else if (rozliv4 != 0 && (scale_last_ok_ms[3] == 0 || (tnow - scale_last_ok_ms[3]) > FILL_SCALE_STALE_MS)) { rozliv4 = 0; }
	if (fill_dose_hit[4] || (rozliv5 != 0 && av[4] >= dose_lim)) { fill_dose_hit[4] = 1; rozliv5 = 0; }
	else if (rozliv5 != 0 && (scale_last_ok_ms[4] == 0 || (tnow - scale_last_ok_ms[4]) > FILL_SCALE_STALE_MS)) { rozliv5 = 0; }
}
//----------------------------------------------------------------------------------------
	if(flag1==1&&flag2==1&&flag3==1&&flag4==1&&flag5==1)
	{
		flagi++;
	}
//----------------------------------------------------------------------------------------
	if(rozliv1==0&&dimona_knopka==1)
	{
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_SET);  //ENA
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);  //DIR
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET); //PULSE
	
	}
	
		if(rozliv1==1&&d1>spusk2-1&&rs1==1)
	{
	
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_8);
	}
	
			if(rozliv1==1&&rozliv_count1>ROZLIV_FAST_PERIOD&&rs1==0&&d1>spusk2-1)
	{
	r1++;
	rozliv_count1=0;
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_8);
	}
	
				if(rozliv1==2&&rozliv_count1>ROZLIV_SLOW_PERIOD)
	{
	rs1=0;
	rozliv_count1=0;
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_8);
	
	}

				if(rozliv1==3&&rozliv_count1>ROZLIV_SOFT_PERIOD&&d1>spusk2-1)
	{
	rozliv_count1=0;
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_8);
	}
	
//----------------------------------------------------------------------------------------
	
	if(rozliv2==0&&dimona_knopka==1)
	{
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_12,GPIO_PIN_RESET);
	}
	
		if(rozliv2==1&&d2>spusk2-1&&rs2==1)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
	}
	
			if(rozliv2==1&&rozliv_count2>ROZLIV_FAST_PERIOD&&rs2==0&&d2>spusk2-1)
	{
	r2++;
	rozliv_count2=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
	}
	
				if(rozliv2==2&&rozliv_count2>ROZLIV_SLOW_PERIOD)
	{
	rs2=0;
	rozliv_count2=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
	}

				if(rozliv2==3&&rozliv_count2>ROZLIV_SOFT_PERIOD&&d2>spusk2-1)
	{
	rozliv_count2=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
	}
	
//----------------------------------------------------------------------------------------	
		
	if(rozliv3==0&&dimona_knopka==1)
	{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_5,GPIO_PIN_RESET);
	}
	
		if(rozliv3==1&&d3>spusk2-1&&rs3==1)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
	}
	
			if(rozliv3==1&&rozliv_count3>ROZLIV_FAST_PERIOD&&rs3==0&&d3>spusk2-1)
	{
	r3++;
	rozliv_count3=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
	}
	
				if(rozliv3==2&&rozliv_count3>ROZLIV_SLOW_PERIOD)
	{
	rs3=0;
	rozliv_count3=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
	}

				if(rozliv3==3&&rozliv_count3>ROZLIV_SOFT_PERIOD&&d3>spusk2-1)
	{
	rozliv_count3=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
	}
	
//----------------------------------------------------------------------------------------	
	if(rozliv4==0&&dimona_knopka==1)
	{
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);
	}
	
		if(rozliv4==1&&d4>spusk2-1&&rs4==1)
	{
	
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_6);
	}
	
			if(rozliv4==1&&rozliv_count4>ROZLIV_FAST_PERIOD&&rs4==0&&d4>spusk2-1)
	{
	r4++;
	rozliv_count4=0;
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_6);
	}
	
				if(rozliv4==2&&rozliv_count4>ROZLIV_SLOW_PERIOD)
	{
	rs4=0;
	rozliv_count4=0;
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_6);
	}

				if(rozliv4==3&&rozliv_count4>ROZLIV_SOFT_PERIOD&&d4>spusk2-1)
	{
	rozliv_count4=0;
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_6);
	}
	
//----------------------------------------------------------------------------------------
		if(rozliv5==0&&dimona_knopka==1)
	{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_2,GPIO_PIN_RESET);
	}
	
		if(rozliv5==1&&d5>spusk2-1&&rs5==1)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_2);
	}
	
			if(rozliv5==1&&rozliv_count5>ROZLIV_FAST_PERIOD&&rs5==0&&d5>spusk2-1)
	{
	r5++;
	rozliv_count5=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_2);
	}
	
				if(rozliv5==2&&rozliv_count5>ROZLIV_SLOW_PERIOD)
	{
	rs5=0;
	rozliv_count5=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_2);
	}

				if(rozliv5==3&&rozliv_count5>ROZLIV_SOFT_PERIOD&&d5>spusk2-1)
	{
	rozliv_count5=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_2);
	}
	
//----------------------------------------------------------------------------------------
		if(flag1==1)
	{
		if(a1==0)
		{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_14,GPIO_PIN_SET);
		//updown1=0;
		a1=1;
		}
			d1=0;
	}
		if(flag1==0&&updown1==1)
	{
	a1=0;
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_14);
		d1--;
	}
	
		if(d1<spusk&&flag_rozliv1==1&&updown1==3)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_14);
		d1++;
	}
	
			if(d1>spusk2&&flag_rozliv1==1&&updown1==2)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_14);
		d1--;
	}
	
				if(d1>ss2[0]&&updown1==4&&verx_count1>ss[0])
	{
	verx_count1=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_14);
		d1--;
	}
	
//----------------------------------------------------------------------------------------
		if(flag2==1)
	{
		if(a2==0)
		{
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);
		//updown1=0;
		a2=1;
		}
			d2=0;
	}
		if(flag2==0&&updown2==1)
	{
	a2=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_14);
		d2--;
	}
	
		if(d2<spusk&&flag_rozliv2==1&&updown2==3)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_14);
		d2++;
	}
	
			if(d2>spusk2&&flag_rozliv2==1&&updown2==2)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_14);
		d2--;
	}
	
					if(d2>ss2[1]&&updown2==4&&verx_count2>ss[1])
	{
	verx_count2=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_14);
		d2--;
	}

//----------------------------------------------------------------------------------------
		if(flag3==1)
	{
		if(a3==0)
		{
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_SET);
		a3=1;
		}
			d3=0;
	}
		if(flag3==0&&updown3==1)
	{
	a3=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_12);
		d3--;
	}
	
		if(d3<spusk&&flag_rozliv3==1&&updown3==3)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_12);
		d3++;
	}
	
			if(d3>spusk2&&flag_rozliv3==1&&updown3==2)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_12);
		d3--;
	}
	
						if(d3>ss2[2]&&updown3==4&&verx_count3>ss[2])
	{
	verx_count3=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_12);
		d3--;
	}
//----------------------------------------------------------------------------------------
			if(flag4==1)
	{
		if(a4==0)
		{
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13,GPIO_PIN_SET);
		a4=1;
		}
			d4=0;
	}
		if(flag4==0&&updown4==1)
	{
	a4=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_13);
		d4--;
	}
	
		if(d4<spusk&&flag_rozliv4==1&&updown4==3)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_13);
		d4++;
	}
	
			if(d4>spusk2&&flag_rozliv4==1&&updown4==2)
	{
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_13);
		d4--;
	}
							if(d4>ss2[3]&&updown4==4&&verx_count4>ss[3])
	{
	verx_count4=0;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_13);
		d4--;
	}
//----------------------------------------------------------------------------------------
				if(flag5==1)
	{
		if(a5==0)
		{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);
		a5=1;
		}
			d5=0;
	}
		if(flag5==0&&updown5==1)
	{
	a5=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_15);
		d5--;
	}
	
		if(d5<spusk&&flag_rozliv5==1&&updown5==3)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_15);
		d5++;
	}
	
			if(d5>spusk2&&flag_rozliv5==1&&updown5==2)
	{
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_15);
		d5--;
	}
	
						if(d5>ss2[4]&&updown5==4&&verx_count5>ss[4])
	{
	verx_count5=0;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_15);
		d5--;
	}
//----------------------------------------------------------------------------------------
	if(dimona_knopka==0)
	{
	//1
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_8);
	//2
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_12);
	//3
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
	//4
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_6);
	//5
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,GPIO_PIN_RESET);
	HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_2);
	}

	/* USB масс: ~раз в 5 мс, работает и во время Delay/tare (TIM6 жив) */
	usb_mass_poll_div++;
	if (usb_mass_poll_div >= USB_MASS_POLL_DIV) {
		usb_mass_poll_div = 0;
		usb_mass_poll();
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_TIM_Base_Start_IT(&htim7);
	
	stepper=4;
	HAL_Delay(10000);
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		modbus_process(); // Опрашиваем 4 весов по кругу
		/* usb_mass_poll: в TIM6, не здесь: иначе Delay/tare режут поток */
		
			if (flag_krutka==1&&flag_vyxod==0&&vxodsec>3000&&rezhim==0&&flag1==1&&flag2==1&&flag3==1&&flag4==1)//&&flag5==1
	{
		flag_krutka=0;
		krutka=metka+640;
		if(krutka>6399)
		{
			krutka=0;
		}
		butylka++;
		vxodsec=0;
		HAL_Delay(500);
		carousel_steps_left=640;
		stepper=55;
		HAL_Delay(1500);
	}
	
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2) == GPIO_PIN_RESET)
	{
		if (tare_button_press_start == 0) {
			tare_button_press_start = HAL_GetTick();
		} else if (tare_button_done == 0 &&
		           (HAL_GetTick() - tare_button_press_start) > 2000) {
			/* Удержание ≥2 с: только peeling всех весов (не калибровка!).
			 * Один раз за нажатие: даже если кнопка залипнет. */
			if (rezhim == 0)
			{
				tare_button_done = 1;
				last_tare_time = HAL_GetTick();
				tare_all_scales();
			}
		}
	}
	else
	{
		tare_button_press_start = 0;
		tare_button_done = 0;
	}
	if(rezhim==0&&butylka<(TARGET_BOTTLES+1))
	{
		
		if(butylka==TARGET_BOTTLES)rezhim=1;
		flag_krutka=1;
	}
	
	if(rezhim==1&&butylka==TARGET_BOTTLES&&stepper==0&&fill_pos_ready)
	{
		fill_pos_ready=0;
		// фоновый опрос Modbus. Поэтому переменные av[0..4] сейчас хранят старый вес!
		// 1. Обязательная пауза 1000мс для механической стабилизации весов после вращения карусели
		uint32_t update_start = HAL_GetTick();
		while (HAL_GetTick() - update_start < 1200) {
			modbus_process();
		}

		// 2. После стабилизации гарантируем получение свежих данных (ждем до 1200мс)
		update_start = HAL_GetTick();
		for(int i=0; i<5; i++) scale_updated[i] = 0;
		while (HAL_GetTick() - update_start < 1200) {
			modbus_process();
			if (scale_updated[0] && scale_updated[1] && scale_updated[2] && scale_updated[3] && scale_updated[4]) {
				break; // Все весы успешно опрошены!
			}
		}

		// Ожидаем завершения любых текущих транзакций Modbus
		HAL_Delay(100);

		butylka=0; // СБРОС СЧЕТЧ�?КА БУТЫЛОК! Без этого станок будет наливать бесконечно!

		butyl[0]=av[0]; // ПРОГРАММНАЯ ТАРА: запоминаем абсолютный вес пустого флакона
		butyl[1]=av[1];
		butyl[2]=av[2];
		butyl[3]=av[3];
		butyl[4]=av[4];

		/* Только свежий опрос + вес > порога: иначе не включаем колонну */
		if(scale_updated[0] && butyl[0]>200)
		{
			zakr1=1;
			flag_rozliv1=1;
			alibek_metka = 1;
		}
		else flag_rozliv1=0;
		
		if(scale_updated[1] && butyl[1]>200)
		{
		zakr2=1;
		flag_rozliv2=1;
		}
		else flag_rozliv2=0;
		
		if(scale_updated[2] && butyl[2]>200)
		{
		zakr3=1;
		flag_rozliv3=1;
		}
		else flag_rozliv3=0;
		
		if(scale_updated[3] && butyl[3]>200)
		{
		zakr4=1;
		flag_rozliv4=1;
		}
		else flag_rozliv4=0;
		
		if(scale_updated[4] && butyl[4]>200)
		{
		flag_rozliv5=1; /* зажим 5-й колонны не используется */
		}
		else flag_rozliv5=0;

		set_tare_active_scales();
		
		/* Ждём стабилизации после peeling, опрашивая весы */
		{
			uint32_t tare_wait_start = HAL_GetTick();
			while (HAL_GetTick() - tare_wait_start < 1000) {
				modbus_process();
			}
		}
		
		if(flag_rozliv1==1)updown1=3;
		if(flag_rozliv2==1)updown2=3;
		if(flag_rozliv3==1)updown3=3;
		if(flag_rozliv4==1)updown4=3;
		if(flag_rozliv5==1)updown5=3;

		{
			int si;
			uint32_t tnow = HAL_GetTick();
			for (si = 0; si < 5; si++) {
				flag_count[si] = 0;
				scale_last_ok_ms[si] = tnow;
				fill_dose_hit[si] = 0;
			}
			rozliv1 = 0;
			rozliv2 = 0;
			rozliv3 = 0;
			rozliv4 = 0;
			rozliv5 = 0;
			fill_start_ms = tnow;
		}
		rezhim=3;
	}
	
		if(rezhim==3)
	{
		
		vesy[0] = av[0]; // Аппаратная тара: av[i] уже показывает чистый вес
		vesy[1] = av[1];
		vesy[2] = av[2];
		vesy[3] = av[3];
		vesy[4] = av[4];

		update_fill_channel(vesy[0], &flag_rozliv1, &flag_count[0], &updown1, &rozliv1, &zakr1, d1, &ss2[0], 0);
		update_fill_channel(vesy[1], &flag_rozliv2, &flag_count[1], &updown2, &rozliv2, &zakr2, d2, &ss2[1], 1);
		update_fill_channel(vesy[2], &flag_rozliv3, &flag_count[2], &updown3, &rozliv3, &zakr3, d3, &ss2[2], 2);
		update_fill_channel(vesy[3], &flag_rozliv4, &flag_count[3], &updown4, &rozliv4, &zakr4, d4, &ss2[3], 3);
		update_fill_channel(vesy[4], &flag_rozliv5, &flag_count[4], &updown5, &rozliv5, &zakr5, d5, &ss2[4], 4);

		if(flag_rozliv1==0&&flag_rozliv2==0&&flag_rozliv3==0&&flag_rozliv4==0&&flag_rozliv5==0) rezhim=4;
		

	}
	
		if(rezhim==4)
	{
		/* До отмены тары: зафиксировать нетто налива и отдать по USB со st=2 */
		{
			int ui;
			for (ui = 0; ui < 5; ui++) {
				usb_fill_latch[ui] = vesy[ui];
			}
			usb_fill_hold = 40; /* несколько пакетов, пока Pi успеет зафиксировать */
			usb_mass_request_send();
		}

		cancel_tare_all_scales();
		
		zakr3=0;
		zakr4=0;
		zakr5=0;
		flag_rozliv1=0;
		flag_rozliv2=0;
		flag_rozliv3=0;
		flag_rozliv4=0;
		flag_rozliv5=0;
		updown1=1;
		updown2=1;
		updown3=1;
		updown4=1;
		updown5=1;
		rezhim=0;
	}
		
		
			


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 167;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

static void MX_TIM7_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 167;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 99;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2|GPIO_PIN_14|GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_10
                          |GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_10|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE14 PE0 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_14|GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE3 PE5 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE4 PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC13 PC15 PC1 PC2 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC3 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA7 PA8 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_10
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC6 PC7 PC8
                           PC9 PC10 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB10 PB13 PB14
                           PB15 PB4 PB6 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_10|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PE7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD11 PD13 PD0
                           PD1 PD3 PD5 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    if (modbus_state == MODBUS_WAIT_RESPONSE) {
      if (rs485_rx_index == 0) {
        if (rs485_rx_byte == modbus_ids[current_sensor_index]) {
          rs485_rx_buffer[0] = rs485_rx_byte;
          rs485_rx_index = 1;
        }
        HAL_UART_Receive_IT(&huart2, &rs485_rx_byte, 1);
      } else if (rs485_rx_index < 9) {
        rs485_rx_buffer[rs485_rx_index] = rs485_rx_byte;
        rs485_rx_index++;
        if (rs485_rx_index == 9) {
          rs485_response_complete = 1;
        } else {
          HAL_UART_Receive_IT(&huart2, &rs485_rx_byte, 1);
        }
      }
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */