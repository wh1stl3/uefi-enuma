/*
 * Host-side test for RamoopsLogLib's console-zone ring, mirroring the firmware AppendByte logic one-to-one.
 *
 * It models the 2 MiB console zone as a byte array and checks three properties:
 *   1. fewer than DATA_SIZE bytes: start == size == bytes written, data laid out in order;
 *   2. exactly full then a wrap: start/size match Linux persistent_ram semantics, and reading back in Linux's order
 *      (data[start..size) then data[0..start)) reproduces the last DATA_SIZE bytes of the stream;
 *   3. a bad header signature or start >= DATA_SIZE makes the next append reset the header.
 *
 * Build (WSL, clang or gcc):
 *   cc -O2 -Wall -Wextra -o test_ramoops_log test_ramoops_log.c && ./test_ramoops_log
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAMOOPS_CONSOLE_SIZE 0x200000u
#define RAMOOPS_SIGNATURE    0x43474244u /* "DBGC" */
#define RAMOOPS_HEADER_SIZE  12u
#define RAMOOPS_DATA_SIZE    (RAMOOPS_CONSOLE_SIZE - RAMOOPS_HEADER_SIZE)

/* The zone is just RAMOOPS_CONSOLE_SIZE bytes: a 12-byte persistent_ram_buffer header, then the data ring. */
static uint8_t gZone[RAMOOPS_CONSOLE_SIZE];

static uint32_t
ReadU32 (const uint8_t *P)
{
  return (uint32_t)P[0] | ((uint32_t)P[1] << 8) | ((uint32_t)P[2] << 16) | ((uint32_t)P[3] << 24);
}

static void
WriteU32 (uint8_t *P, uint32_t V)
{
  P[0] = (uint8_t)V;
  P[1] = (uint8_t)(V >> 8);
  P[2] = (uint8_t)(V >> 16);
  P[3] = (uint8_t)(V >> 24);
}

static void
ResetHeader (void)
{
  WriteU32 (&gZone[0], RAMOOPS_SIGNATURE);
  WriteU32 (&gZone[4], 0);
  WriteU32 (&gZone[8], 0);
}

/* One-to-one mirror of the firmware AppendByte (RamoopsLogLib.c). */
static void
AppendByte (uint8_t Byte)
{
  uint32_t Start = ReadU32 (&gZone[4]);
  uint32_t Size  = ReadU32 (&gZone[8]);

  if (ReadU32 (&gZone[0]) != RAMOOPS_SIGNATURE || Start >= RAMOOPS_DATA_SIZE || Size > RAMOOPS_DATA_SIZE) {
    ResetHeader ();
    Start = 0;
    Size  = 0;
  }

  gZone[RAMOOPS_HEADER_SIZE + Start] = Byte;

  Start = (Start + 1 == RAMOOPS_DATA_SIZE) ? 0 : Start + 1;
  if (Size < RAMOOPS_DATA_SIZE) {
    Size++;
  }

  WriteU32 (&gZone[4], Start);
  WriteU32 (&gZone[8], Size);
}

static int gFailures = 0;

#define CHECK(Cond, Msg)                                            \
  do {                                                              \
    if (!(Cond)) {                                                  \
      fprintf (stderr, "FAIL: %s\n", Msg);                          \
      gFailures++;                                                  \
    } else {                                                        \
      printf ("ok: %s\n", Msg);                                     \
    }                                                               \
  } while (0)

/* Reconstruct the last DATA_SIZE bytes using Linux persistent_ram_save_old's read order. */
static void
ReadOld (uint32_t Start, uint32_t Size, uint8_t *Out)
{
  uint32_t First = Size - Start;          /* data[start..size) */
  uint32_t Second = Start;                /* data[0..start)   */
  memcpy (Out, &gZone[RAMOOPS_HEADER_SIZE + Start], First);
  memcpy (Out + First, &gZone[RAMOOPS_HEADER_SIZE], Second);
}

int
main (void)
{
  /* Test 1: fewer than DATA_SIZE bytes. */
  memset (gZone, 0, sizeof (gZone));
  ResetHeader ();

  const uint32_t N1 = 1000;
  uint8_t Stream1[N1];
  for (uint32_t i = 0; i < N1; i++) {
    Stream1[i] = (uint8_t)(i * 7 + 1);
    AppendByte (Stream1[i]);
  }

  CHECK (ReadU32 (&gZone[4]) == N1, "test1: start == bytes written");
  CHECK (ReadU32 (&gZone[8]) == N1, "test1: size == bytes written");
  CHECK (memcmp (&gZone[RAMOOPS_HEADER_SIZE], Stream1, N1) == 0, "test1: data laid out in order");

  /* Test 2: exactly full, then wrap. */
  memset (gZone, 0, sizeof (gZone));
  ResetHeader ();

  /* Fill the whole data area with a deterministic pattern. */
  uint8_t *Full = malloc (RAMOOPS_DATA_SIZE);
  if (Full == NULL) {
    fprintf (stderr, "FAIL: malloc of full pattern\n");
    return 2;
  }
  for (uint32_t i = 0; i < RAMOOPS_DATA_SIZE; i++) {
    Full[i] = (uint8_t)(i * 13 + 3);
    AppendByte (Full[i]);
  }

  CHECK (ReadU32 (&gZone[4]) == 0, "test2: after exact fill start wraps to 0");
  CHECK (ReadU32 (&gZone[8]) == RAMOOPS_DATA_SIZE, "test2: after exact fill size == DATA_SIZE");

  /* Append N2 more bytes so the ring wraps over the oldest bytes. */
  const uint32_t N2 = 100;
  for (uint32_t i = 0; i < N2; i++) {
    AppendByte ((uint8_t)(0x80 + i));
  }

  CHECK (ReadU32 (&gZone[4]) == N2, "test2: after wrap start == N2");
  CHECK (ReadU32 (&gZone[8]) == RAMOOPS_DATA_SIZE, "test2: after wrap size stays DATA_SIZE");

  /* The last DATA_SIZE bytes of the stream: Full[N2..] then the N2 wrapped bytes. */
  uint8_t *Expected = malloc (RAMOOPS_DATA_SIZE);
  uint8_t *Actual = malloc (RAMOOPS_DATA_SIZE);
  if (Expected == NULL || Actual == NULL) {
    fprintf (stderr, "FAIL: malloc of expected/actual\n");
    return 2;
  }
  memcpy (Expected, &Full[N2], RAMOOPS_DATA_SIZE - N2);
  for (uint32_t i = 0; i < N2; i++) {
    Expected[RAMOOPS_DATA_SIZE - N2 + i] = (uint8_t)(0x80 + i);
  }

  ReadOld (ReadU32 (&gZone[4]), ReadU32 (&gZone[8]), Actual);
  CHECK (memcmp (Actual, Expected, RAMOOPS_DATA_SIZE) == 0, "test2: Linux read order reproduces the last DATA_SIZE bytes");

  free (Full);
  free (Expected);
  free (Actual);

  /* Test 3: corrupt header self-resets. */
  memset (gZone, 0, sizeof (gZone));
  WriteU32 (&gZone[0], 0xDEADBEEF);     /* bad signature */
  WriteU32 (&gZone[4], 12);             /* arbitrary non-zero start */
  WriteU32 (&gZone[8], 55);
  AppendByte ('X');
  CHECK (ReadU32 (&gZone[0]) == RAMOOPS_SIGNATURE, "test3a: bad signature resets header");
  CHECK (ReadU32 (&gZone[4]) == 1, "test3a: after reset +1 byte, start == 1");
  CHECK (ReadU32 (&gZone[8]) == 1, "test3a: after reset +1 byte, size == 1");
  CHECK (gZone[RAMOOPS_HEADER_SIZE] == 'X', "test3a: the appended byte landed at the front");

  /* start >= DATA_SIZE also self-resets, even with a valid signature. */
  memset (gZone, 0, sizeof (gZone));
  ResetHeader ();
  WriteU32 (&gZone[4], RAMOOPS_DATA_SIZE + 5); /* start out of range */
  AppendByte ('Y');
  CHECK (ReadU32 (&gZone[0]) == RAMOOPS_SIGNATURE, "test3b: start >= DATA_SIZE resets header");
  CHECK (ReadU32 (&gZone[4]) == 1, "test3b: after reset +1 byte, start == 1");
  CHECK (ReadU32 (&gZone[8]) == 1, "test3b: after reset +1 byte, size == 1");
  CHECK (gZone[RAMOOPS_HEADER_SIZE] == 'Y', "test3b: the appended byte landed at the front");

  if (gFailures == 0) {
    printf ("ALL TESTS PASSED\n");
    return 0;
  }

  fprintf (stderr, "%d CHECK(S) FAILED\n", gFailures);
  return 1;
}
