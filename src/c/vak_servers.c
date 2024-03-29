#include <stdlib.h>

#include "vak.h"

/* List of roughtime servers from
 * https://github.com/cloudflare/roughtime/blob/master/ecosystem.json
 * with additional Netnod's test servers.
 *
 * Note tht falseticker.roughtime.netnod.se are for testing and will
 * usually return bad time.  This is intended for testing of the
 * algorithms used in "vad är klockan".
 *
 * Note that the first set of servers is repeated a couple times so
 * that they will be a majority compared to the falsetickers at the
 * end.
 */

static struct vak_server const server_list[] = {
#if 0
    // { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 5, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 5, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 7, { 0x88,0x6b,0x14,0x1c,0x0d,0x38,0xb9,0x4b,0xaa,0x79,0xe6,0x7c,0xae,0x4f,0x07,0x3e,0x83,0x70,0x04,0xf0,0x7f,0x96,0x54,0x20,0x4f,0xb9,0xb9,0x42,0x6e,0x43,0xc7,0x2b } }, /* lab.roughtime.netnod.se */

    // { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 7, { 0x88,0x6b,0x14,0x1c,0x0d,0x38,0xb9,0x4b,0xaa,0x79,0xe6,0x7c,0xae,0x4f,0x07,0x3e,0x83,0x70,0x04,0xf0,0x7f,0x96,0x54,0x20,0x4f,0xb9,0xb9,0x42,0x6e,0x43,0xc7,0x2b } }, /* lab.roughtime.netnod.se */

    // { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 7, { 0x88,0x6b,0x14,0x1c,0x0d,0x38,0xb9,0x4b,0xaa,0x79,0xe6,0x7c,0xae,0x4f,0x07,0x3e,0x83,0x70,0x04,0xf0,0x7f,0x96,0x54,0x20,0x4f,0xb9,0xb9,0x42,0x6e,0x43,0xc7,0x2b } }, /* lab.roughtime.netnod.se */

    // { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 7, { 0x88,0x6b,0x14,0x1c,0x0d,0x38,0xb9,0x4b,0xaa,0x79,0xe6,0x7c,0xae,0x4f,0x07,0x3e,0x83,0x70,0x04,0xf0,0x7f,0x96,0x54,0x20,0x4f,0xb9,0xb9,0x42,0x6e,0x43,0xc7,0x2b } }, /* lab.roughtime.netnod.se */

    { "194.58.207.197", 2002, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2000, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2001, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2003, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2004, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2005, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2006, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2007, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2008, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2009, 7, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
#else
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "192.36.143.134", 2002, 7, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.198", 2002, 7, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 7, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
#endif
};

struct vak_server const **vak_get_servers(void)
{
    struct vak_server const **l;
    unsigned n, i;

    n = sizeof(server_list) / sizeof(server_list[0]);
    l = malloc((n + 1) * sizeof(*l));

    if (l == NULL)
        return NULL;

    for (i = 0; i < n; i++)
        l[i] = &server_list[i];
    l[i] = NULL;

    return (struct vak_server const **)l;
};

struct vak_server const **vak_get_randomized_servers(void)
{
    struct vak_server const **l;
    unsigned i;

    l = (struct vak_server const **)vak_get_servers();
    if (l == NULL)
        return NULL;

    /* find end of list */
    for (i = 0; l[i]; i++)
        ;

    /* iterate backwards and randomly swap position with an element
     * below it.  Leave the element alone if it was going to swap with
     * itself. */
    for (; i > 0; i--) {
        struct vak_server const *t;

        int j = random() % i;

        if (i != j) {
            t = l[i-1];
            l[i-i] = l[j];
            l[j] = t;
        }
    }

    return (struct vak_server const **)l;
}

void vak_servers_del(struct vak_server const **servers)
{
    free((void *)servers);
}
