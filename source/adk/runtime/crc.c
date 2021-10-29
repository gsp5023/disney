// https://github.com/lammertb/libcrc
// https://github.com/lammertb/libcrc/blob/master/LICENSE
/*
* Library: libcrc
* File:    include/checksum.h
* Author:  Lammert Bies
*
* This file is licensed under the MIT License as stated below
*
* Copyright (c) 1999-2016 Lammert Bies
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Description
* -----------
* The headerfile include/checksum.h contains the definitions and prototypes
* for routines that can be used to calculate several kinds of checksums.
*/

#include _PCH
#include "crc.h"

/*
* #define CRC_POLY_xxxx
*
* The constants of the form CRC_POLY_xxxx define the polynomials for some well
* known CRC calculations.
*/

#define CRC_POLY_16 0xA001
#define CRC_POLY_32 0xEDB88320ul
#define CRC_POLY_64 0x42F0E1EBA9EA3693ull

/*
* #define CRC_START_xxxx
*
* The constants of the form CRC_START_xxxx define the values that are used for
* initialization of a CRC value for common used calculation methods.
*/

#define CRC_START_8 0x00
#define CRC_START_16 0x0000
#define CRC_START_MODBUS 0xFFFF
#define CRC_START_XMODEM 0x0000
#define CRC_START_CCITT_1D0F 0x1D0F
#define CRC_START_CCITT_FFFF 0xFFFF
#define CRC_START_KERMIT 0x0000
#define CRC_START_SICK 0x0000
#define CRC_START_DNP 0x0000
#define CRC_START_32 0xFFFFFFFFul
#define CRC_START_64_ECMA 0x0000000000000000ull
#define CRC_START_64_WE 0xFFFFFFFFFFFFFFFFull

/*
* unsigned char *checksum_NMEA( const unsigned char *input_str, unsigned char *result );
*
* The function checksum_NMEA() calculates the checksum of a valid NMEA string.
* The routine does not try to validate the string itself. A leading '$' will
* be ignored, as this character is part of the NMEA sentence, but not part of
* the checksum calculation. The calculation stops, whenever a linefeed,
* carriage return, '*' or end of string is scanned.
*
* Because there is no NMEA syntax checking involved, the function always
* returns with succes, unless a NULL pointer is provided as parameter. The
* return value is a pointer to the result buffer provided by the calling
* application, or NULL in case of error.
*
* The result buffer must be at least three characters long. Two for the
* checksum value and the third to store the EOS. The result buffer is not
* filled when an error occurs.
*/

unsigned char * checksum_NMEA(const unsigned char * input_str, unsigned char * result) {
    const unsigned char * ptr;
    unsigned char checksum;

    if (input_str == NULL)
        return NULL;
    if (result == NULL)
        return NULL;

    checksum = 0;
    ptr = (const unsigned char *)input_str;

    if (*ptr == '$')
        ptr++;

    while (*ptr && *ptr != '\r' && *ptr != '\n' && *ptr != '*')
        checksum ^= *ptr++;

    snprintf((char *)result, 3, "%02X", checksum);

    return result;

} /* checksum_NMEA */

static const uint64_t crc_tab64[256] = {
    0x0000000000000000ull,
    0x42F0E1EBA9EA3693ull,
    0x85E1C3D753D46D26ull,
    0xC711223CFA3E5BB5ull,
    0x493366450E42ECDFull,
    0x0BC387AEA7A8DA4Cull,
    0xCCD2A5925D9681F9ull,
    0x8E224479F47CB76Aull,
    0x9266CC8A1C85D9BEull,
    0xD0962D61B56FEF2Dull,
    0x17870F5D4F51B498ull,
    0x5577EEB6E6BB820Bull,
    0xDB55AACF12C73561ull,
    0x99A54B24BB2D03F2ull,
    0x5EB4691841135847ull,
    0x1C4488F3E8F96ED4ull,
    0x663D78FF90E185EFull,
    0x24CD9914390BB37Cull,
    0xE3DCBB28C335E8C9ull,
    0xA12C5AC36ADFDE5Aull,
    0x2F0E1EBA9EA36930ull,
    0x6DFEFF5137495FA3ull,
    0xAAEFDD6DCD770416ull,
    0xE81F3C86649D3285ull,
    0xF45BB4758C645C51ull,
    0xB6AB559E258E6AC2ull,
    0x71BA77A2DFB03177ull,
    0x334A9649765A07E4ull,
    0xBD68D2308226B08Eull,
    0xFF9833DB2BCC861Dull,
    0x388911E7D1F2DDA8ull,
    0x7A79F00C7818EB3Bull,
    0xCC7AF1FF21C30BDEull,
    0x8E8A101488293D4Dull,
    0x499B3228721766F8ull,
    0x0B6BD3C3DBFD506Bull,
    0x854997BA2F81E701ull,
    0xC7B97651866BD192ull,
    0x00A8546D7C558A27ull,
    0x4258B586D5BFBCB4ull,
    0x5E1C3D753D46D260ull,
    0x1CECDC9E94ACE4F3ull,
    0xDBFDFEA26E92BF46ull,
    0x990D1F49C77889D5ull,
    0x172F5B3033043EBFull,
    0x55DFBADB9AEE082Cull,
    0x92CE98E760D05399ull,
    0xD03E790CC93A650Aull,
    0xAA478900B1228E31ull,
    0xE8B768EB18C8B8A2ull,
    0x2FA64AD7E2F6E317ull,
    0x6D56AB3C4B1CD584ull,
    0xE374EF45BF6062EEull,
    0xA1840EAE168A547Dull,
    0x66952C92ECB40FC8ull,
    0x2465CD79455E395Bull,
    0x3821458AADA7578Full,
    0x7AD1A461044D611Cull,
    0xBDC0865DFE733AA9ull,
    0xFF3067B657990C3Aull,
    0x711223CFA3E5BB50ull,
    0x33E2C2240A0F8DC3ull,
    0xF4F3E018F031D676ull,
    0xB60301F359DBE0E5ull,
    0xDA050215EA6C212Full,
    0x98F5E3FE438617BCull,
    0x5FE4C1C2B9B84C09ull,
    0x1D14202910527A9Aull,
    0x93366450E42ECDF0ull,
    0xD1C685BB4DC4FB63ull,
    0x16D7A787B7FAA0D6ull,
    0x5427466C1E109645ull,
    0x4863CE9FF6E9F891ull,
    0x0A932F745F03CE02ull,
    0xCD820D48A53D95B7ull,
    0x8F72ECA30CD7A324ull,
    0x0150A8DAF8AB144Eull,
    0x43A04931514122DDull,
    0x84B16B0DAB7F7968ull,
    0xC6418AE602954FFBull,
    0xBC387AEA7A8DA4C0ull,
    0xFEC89B01D3679253ull,
    0x39D9B93D2959C9E6ull,
    0x7B2958D680B3FF75ull,
    0xF50B1CAF74CF481Full,
    0xB7FBFD44DD257E8Cull,
    0x70EADF78271B2539ull,
    0x321A3E938EF113AAull,
    0x2E5EB66066087D7Eull,
    0x6CAE578BCFE24BEDull,
    0xABBF75B735DC1058ull,
    0xE94F945C9C3626CBull,
    0x676DD025684A91A1ull,
    0x259D31CEC1A0A732ull,
    0xE28C13F23B9EFC87ull,
    0xA07CF2199274CA14ull,
    0x167FF3EACBAF2AF1ull,
    0x548F120162451C62ull,
    0x939E303D987B47D7ull,
    0xD16ED1D631917144ull,
    0x5F4C95AFC5EDC62Eull,
    0x1DBC74446C07F0BDull,
    0xDAAD56789639AB08ull,
    0x985DB7933FD39D9Bull,
    0x84193F60D72AF34Full,
    0xC6E9DE8B7EC0C5DCull,
    0x01F8FCB784FE9E69ull,
    0x43081D5C2D14A8FAull,
    0xCD2A5925D9681F90ull,
    0x8FDAB8CE70822903ull,
    0x48CB9AF28ABC72B6ull,
    0x0A3B7B1923564425ull,
    0x70428B155B4EAF1Eull,
    0x32B26AFEF2A4998Dull,
    0xF5A348C2089AC238ull,
    0xB753A929A170F4ABull,
    0x3971ED50550C43C1ull,
    0x7B810CBBFCE67552ull,
    0xBC902E8706D82EE7ull,
    0xFE60CF6CAF321874ull,
    0xE224479F47CB76A0ull,
    0xA0D4A674EE214033ull,
    0x67C58448141F1B86ull,
    0x253565A3BDF52D15ull,
    0xAB1721DA49899A7Full,
    0xE9E7C031E063ACECull,
    0x2EF6E20D1A5DF759ull,
    0x6C0603E6B3B7C1CAull,
    0xF6FAE5C07D3274CDull,
    0xB40A042BD4D8425Eull,
    0x731B26172EE619EBull,
    0x31EBC7FC870C2F78ull,
    0xBFC9838573709812ull,
    0xFD39626EDA9AAE81ull,
    0x3A28405220A4F534ull,
    0x78D8A1B9894EC3A7ull,
    0x649C294A61B7AD73ull,
    0x266CC8A1C85D9BE0ull,
    0xE17DEA9D3263C055ull,
    0xA38D0B769B89F6C6ull,
    0x2DAF4F0F6FF541ACull,
    0x6F5FAEE4C61F773Full,
    0xA84E8CD83C212C8Aull,
    0xEABE6D3395CB1A19ull,
    0x90C79D3FEDD3F122ull,
    0xD2377CD44439C7B1ull,
    0x15265EE8BE079C04ull,
    0x57D6BF0317EDAA97ull,
    0xD9F4FB7AE3911DFDull,
    0x9B041A914A7B2B6Eull,
    0x5C1538ADB04570DBull,
    0x1EE5D94619AF4648ull,
    0x02A151B5F156289Cull,
    0x4051B05E58BC1E0Full,
    0x87409262A28245BAull,
    0xC5B073890B687329ull,
    0x4B9237F0FF14C443ull,
    0x0962D61B56FEF2D0ull,
    0xCE73F427ACC0A965ull,
    0x8C8315CC052A9FF6ull,
    0x3A80143F5CF17F13ull,
    0x7870F5D4F51B4980ull,
    0xBF61D7E80F251235ull,
    0xFD913603A6CF24A6ull,
    0x73B3727A52B393CCull,
    0x31439391FB59A55Full,
    0xF652B1AD0167FEEAull,
    0xB4A25046A88DC879ull,
    0xA8E6D8B54074A6ADull,
    0xEA16395EE99E903Eull,
    0x2D071B6213A0CB8Bull,
    0x6FF7FA89BA4AFD18ull,
    0xE1D5BEF04E364A72ull,
    0xA3255F1BE7DC7CE1ull,
    0x64347D271DE22754ull,
    0x26C49CCCB40811C7ull,
    0x5CBD6CC0CC10FAFCull,
    0x1E4D8D2B65FACC6Full,
    0xD95CAF179FC497DAull,
    0x9BAC4EFC362EA149ull,
    0x158E0A85C2521623ull,
    0x577EEB6E6BB820B0ull,
    0x906FC95291867B05ull,
    0xD29F28B9386C4D96ull,
    0xCEDBA04AD0952342ull,
    0x8C2B41A1797F15D1ull,
    0x4B3A639D83414E64ull,
    0x09CA82762AAB78F7ull,
    0x87E8C60FDED7CF9Dull,
    0xC51827E4773DF90Eull,
    0x020905D88D03A2BBull,
    0x40F9E43324E99428ull,
    0x2CFFE7D5975E55E2ull,
    0x6E0F063E3EB46371ull,
    0xA91E2402C48A38C4ull,
    0xEBEEC5E96D600E57ull,
    0x65CC8190991CB93Dull,
    0x273C607B30F68FAEull,
    0xE02D4247CAC8D41Bull,
    0xA2DDA3AC6322E288ull,
    0xBE992B5F8BDB8C5Cull,
    0xFC69CAB42231BACFull,
    0x3B78E888D80FE17Aull,
    0x7988096371E5D7E9ull,
    0xF7AA4D1A85996083ull,
    0xB55AACF12C735610ull,
    0x724B8ECDD64D0DA5ull,
    0x30BB6F267FA73B36ull,
    0x4AC29F2A07BFD00Dull,
    0x08327EC1AE55E69Eull,
    0xCF235CFD546BBD2Bull,
    0x8DD3BD16FD818BB8ull,
    0x03F1F96F09FD3CD2ull,
    0x41011884A0170A41ull,
    0x86103AB85A2951F4ull,
    0xC4E0DB53F3C36767ull,
    0xD8A453A01B3A09B3ull,
    0x9A54B24BB2D03F20ull,
    0x5D45907748EE6495ull,
    0x1FB5719CE1045206ull,
    0x919735E51578E56Cull,
    0xD367D40EBC92D3FFull,
    0x1476F63246AC884Aull,
    0x568617D9EF46BED9ull,
    0xE085162AB69D5E3Cull,
    0xA275F7C11F7768AFull,
    0x6564D5FDE549331Aull,
    0x279434164CA30589ull,
    0xA9B6706FB8DFB2E3ull,
    0xEB46918411358470ull,
    0x2C57B3B8EB0BDFC5ull,
    0x6EA7525342E1E956ull,
    0x72E3DAA0AA188782ull,
    0x30133B4B03F2B111ull,
    0xF7021977F9CCEAA4ull,
    0xB5F2F89C5026DC37ull,
    0x3BD0BCE5A45A6B5Dull,
    0x79205D0E0DB05DCEull,
    0xBE317F32F78E067Bull,
    0xFCC19ED95E6430E8ull,
    0x86B86ED5267CDBD3ull,
    0xC4488F3E8F96ED40ull,
    0x0359AD0275A8B6F5ull,
    0x41A94CE9DC428066ull,
    0xCF8B0890283E370Cull,
    0x8D7BE97B81D4019Full,
    0x4A6ACB477BEA5A2Aull,
    0x089A2AACD2006CB9ull,
    0x14DEA25F3AF9026Dull,
    0x562E43B4931334FEull,
    0x913F6188692D6F4Bull,
    0xD3CF8063C0C759D8ull,
    0x5DEDC41A34BBEEB2ull,
    0x1F1D25F19D51D821ull,
    0xD80C07CD676F8394ull,
    0x9AFCE626CE85B507ull};

/*
* uint64_t crc_64_ecma( const unsigned char *input_str, size_t num_bytes );
*
* The function crc_64_ecma() calculates in one pass the ECMA 64 bit CRC value
* for a byte string that is passed to the function together with a parameter
* indicating the length.
*/

uint64_t crc_64_ecma(const unsigned char * input_str, size_t num_bytes) {
    uint64_t crc;
    const unsigned char * ptr;
    size_t a;

    crc = CRC_START_64_ECMA;
    ptr = input_str;

    if (ptr != NULL)
        for (a = 0; a < num_bytes; a++) {
            crc = (crc << 8) ^ crc_tab64[((crc >> 56) ^ (uint64_t)*ptr++) & 0x00000000000000FFull];
        }

    return crc;

} /* crc_64_ecma */

/*
* uint64_t crc_64_we( const unsigned char *input_str, size_t num_bytes );
*
* The function crc_64_we() calculates in one pass the CRC64-WE 64 bit CRC
* value for a byte string that is passed to the function together with a
* parameter indicating the length.
*/

uint64_t crc_64_we(const unsigned char * input_str, size_t num_bytes) {
    uint64_t crc;
    const unsigned char * ptr;
    size_t a;

    crc = CRC_START_64_WE;
    ptr = input_str;

    if (ptr != NULL)
        for (a = 0; a < num_bytes; a++) {
            crc = (crc << 8) ^ crc_tab64[((crc >> 56) ^ (uint64_t)*ptr++) & 0x00000000000000FFull];
        }

    return crc ^ 0xFFFFFFFFFFFFFFFFull;

} /* crc_64_we */

/*
* uint64_t update_crc_64_ecma( uint64_t crc, unsigned char c );
*
* The function update_crc_64_ecma() calculates a new CRC-64 value based on the
* previous value of the CRC and the next byte of the data to be checked.
*/

uint64_t update_crc_64_ecma(uint64_t crc, unsigned char c) {
    return (crc << 8) ^ crc_tab64[((crc >> 56) ^ (uint64_t)c) & 0x00000000000000FFull];

} /* update_crc_64 */

static const uint32_t crc_tab32[256] = {
    0x00000000ul,
    0x77073096ul,
    0xEE0E612Cul,
    0x990951BAul,
    0x076DC419ul,
    0x706AF48Ful,
    0xE963A535ul,
    0x9E6495A3ul,
    0x0EDB8832ul,
    0x79DCB8A4ul,
    0xE0D5E91Eul,
    0x97D2D988ul,
    0x09B64C2Bul,
    0x7EB17CBDul,
    0xE7B82D07ul,
    0x90BF1D91ul,
    0x1DB71064ul,
    0x6AB020F2ul,
    0xF3B97148ul,
    0x84BE41DEul,
    0x1ADAD47Dul,
    0x6DDDE4EBul,
    0xF4D4B551ul,
    0x83D385C7ul,
    0x136C9856ul,
    0x646BA8C0ul,
    0xFD62F97Aul,
    0x8A65C9ECul,
    0x14015C4Ful,
    0x63066CD9ul,
    0xFA0F3D63ul,
    0x8D080DF5ul,
    0x3B6E20C8ul,
    0x4C69105Eul,
    0xD56041E4ul,
    0xA2677172ul,
    0x3C03E4D1ul,
    0x4B04D447ul,
    0xD20D85FDul,
    0xA50AB56Bul,
    0x35B5A8FAul,
    0x42B2986Cul,
    0xDBBBC9D6ul,
    0xACBCF940ul,
    0x32D86CE3ul,
    0x45DF5C75ul,
    0xDCD60DCFul,
    0xABD13D59ul,
    0x26D930ACul,
    0x51DE003Aul,
    0xC8D75180ul,
    0xBFD06116ul,
    0x21B4F4B5ul,
    0x56B3C423ul,
    0xCFBA9599ul,
    0xB8BDA50Ful,
    0x2802B89Eul,
    0x5F058808ul,
    0xC60CD9B2ul,
    0xB10BE924ul,
    0x2F6F7C87ul,
    0x58684C11ul,
    0xC1611DABul,
    0xB6662D3Dul,
    0x76DC4190ul,
    0x01DB7106ul,
    0x98D220BCul,
    0xEFD5102Aul,
    0x71B18589ul,
    0x06B6B51Ful,
    0x9FBFE4A5ul,
    0xE8B8D433ul,
    0x7807C9A2ul,
    0x0F00F934ul,
    0x9609A88Eul,
    0xE10E9818ul,
    0x7F6A0DBBul,
    0x086D3D2Dul,
    0x91646C97ul,
    0xE6635C01ul,
    0x6B6B51F4ul,
    0x1C6C6162ul,
    0x856530D8ul,
    0xF262004Eul,
    0x6C0695EDul,
    0x1B01A57Bul,
    0x8208F4C1ul,
    0xF50FC457ul,
    0x65B0D9C6ul,
    0x12B7E950ul,
    0x8BBEB8EAul,
    0xFCB9887Cul,
    0x62DD1DDFul,
    0x15DA2D49ul,
    0x8CD37CF3ul,
    0xFBD44C65ul,
    0x4DB26158ul,
    0x3AB551CEul,
    0xA3BC0074ul,
    0xD4BB30E2ul,
    0x4ADFA541ul,
    0x3DD895D7ul,
    0xA4D1C46Dul,
    0xD3D6F4FBul,
    0x4369E96Aul,
    0x346ED9FCul,
    0xAD678846ul,
    0xDA60B8D0ul,
    0x44042D73ul,
    0x33031DE5ul,
    0xAA0A4C5Ful,
    0xDD0D7CC9ul,
    0x5005713Cul,
    0x270241AAul,
    0xBE0B1010ul,
    0xC90C2086ul,
    0x5768B525ul,
    0x206F85B3ul,
    0xB966D409ul,
    0xCE61E49Ful,
    0x5EDEF90Eul,
    0x29D9C998ul,
    0xB0D09822ul,
    0xC7D7A8B4ul,
    0x59B33D17ul,
    0x2EB40D81ul,
    0xB7BD5C3Bul,
    0xC0BA6CADul,
    0xEDB88320ul,
    0x9ABFB3B6ul,
    0x03B6E20Cul,
    0x74B1D29Aul,
    0xEAD54739ul,
    0x9DD277AFul,
    0x04DB2615ul,
    0x73DC1683ul,
    0xE3630B12ul,
    0x94643B84ul,
    0x0D6D6A3Eul,
    0x7A6A5AA8ul,
    0xE40ECF0Bul,
    0x9309FF9Dul,
    0x0A00AE27ul,
    0x7D079EB1ul,
    0xF00F9344ul,
    0x8708A3D2ul,
    0x1E01F268ul,
    0x6906C2FEul,
    0xF762575Dul,
    0x806567CBul,
    0x196C3671ul,
    0x6E6B06E7ul,
    0xFED41B76ul,
    0x89D32BE0ul,
    0x10DA7A5Aul,
    0x67DD4ACCul,
    0xF9B9DF6Ful,
    0x8EBEEFF9ul,
    0x17B7BE43ul,
    0x60B08ED5ul,
    0xD6D6A3E8ul,
    0xA1D1937Eul,
    0x38D8C2C4ul,
    0x4FDFF252ul,
    0xD1BB67F1ul,
    0xA6BC5767ul,
    0x3FB506DDul,
    0x48B2364Bul,
    0xD80D2BDAul,
    0xAF0A1B4Cul,
    0x36034AF6ul,
    0x41047A60ul,
    0xDF60EFC3ul,
    0xA867DF55ul,
    0x316E8EEFul,
    0x4669BE79ul,
    0xCB61B38Cul,
    0xBC66831Aul,
    0x256FD2A0ul,
    0x5268E236ul,
    0xCC0C7795ul,
    0xBB0B4703ul,
    0x220216B9ul,
    0x5505262Ful,
    0xC5BA3BBEul,
    0xB2BD0B28ul,
    0x2BB45A92ul,
    0x5CB36A04ul,
    0xC2D7FFA7ul,
    0xB5D0CF31ul,
    0x2CD99E8Bul,
    0x5BDEAE1Dul,
    0x9B64C2B0ul,
    0xEC63F226ul,
    0x756AA39Cul,
    0x026D930Aul,
    0x9C0906A9ul,
    0xEB0E363Ful,
    0x72076785ul,
    0x05005713ul,
    0x95BF4A82ul,
    0xE2B87A14ul,
    0x7BB12BAEul,
    0x0CB61B38ul,
    0x92D28E9Bul,
    0xE5D5BE0Dul,
    0x7CDCEFB7ul,
    0x0BDBDF21ul,
    0x86D3D2D4ul,
    0xF1D4E242ul,
    0x68DDB3F8ul,
    0x1FDA836Eul,
    0x81BE16CDul,
    0xF6B9265Bul,
    0x6FB077E1ul,
    0x18B74777ul,
    0x88085AE6ul,
    0xFF0F6A70ul,
    0x66063BCAul,
    0x11010B5Cul,
    0x8F659EFFul,
    0xF862AE69ul,
    0x616BFFD3ul,
    0x166CCF45ul,
    0xA00AE278ul,
    0xD70DD2EEul,
    0x4E048354ul,
    0x3903B3C2ul,
    0xA7672661ul,
    0xD06016F7ul,
    0x4969474Dul,
    0x3E6E77DBul,
    0xAED16A4Aul,
    0xD9D65ADCul,
    0x40DF0B66ul,
    0x37D83BF0ul,
    0xA9BCAE53ul,
    0xDEBB9EC5ul,
    0x47B2CF7Ful,
    0x30B5FFE9ul,
    0xBDBDF21Cul,
    0xCABAC28Aul,
    0x53B39330ul,
    0x24B4A3A6ul,
    0xBAD03605ul,
    0xCDD70693ul,
    0x54DE5729ul,
    0x23D967BFul,
    0xB3667A2Eul,
    0xC4614AB8ul,
    0x5D681B02ul,
    0x2A6F2B94ul,
    0xB40BBE37ul,
    0xC30C8EA1ul,
    0x5A05DF1Bul,
    0x2D02EF8Dul};

/*
* uint32_t crc_32( const unsigned char *input_str, size_t num_bytes );
*
* The function crc_32() calculates in one pass the common 32 bit CRC value for
* a byte string that is passed to the function together with a parameter
* indicating the length.
*/

uint32_t crc_32(const unsigned char * input_str, size_t num_bytes) {
    uint32_t crc = CRC_START_32;
    const unsigned char * ptr = input_str;

    for (size_t a = 0; a < num_bytes; a++) {
        crc = (crc >> 8) ^ crc_tab32[(crc ^ (uint32_t)*ptr++) & 0x000000FFul];
    }

    return (crc ^ 0xFFFFFFFFul);
}

/*
* uint32_t update_crc_32( uint32_t crc, unsigned char c );
*
* The function update_crc_32() calculates a new CRC-32 value based on the
* previous value of the CRC and the next byte of the data to be checked.
*/

uint32_t update_crc_32(const uint32_t crc, const unsigned char * c, const size_t num_bytes) {
    uint32_t curr_crc = crc;
    for (size_t a = 0; a < num_bytes; a++) {
        curr_crc = (curr_crc >> 8) ^ crc_tab32[(curr_crc ^ (uint32_t)*c++) & 0x000000FFul];
    }
    return (curr_crc ^ 0xFFFFFFFFul);

} /* update_crc_32 */

uint32_t crc_str_32(const char * str) {
    uint32_t crc;

    crc = CRC_START_32;

    while (*str != 0) {
        crc = (crc >> 8) ^ crc_tab32[(crc ^ (uint32_t)*str++) & 0x000000FFul];
    }

    return (crc ^ 0xFFFFFFFFul);
}

uint32_t update_crc_str_32(uint32_t crc, const char * str) {
    while (*str != 0) {
        crc = (crc >> 8) ^ crc_tab32[(crc ^ (uint32_t)*str++) & 0x000000FFul];
    }

    return (crc ^ 0xFFFFFFFFul);
}

static const uint16_t crc_tab16[256] = {
    0x0000u,
    0xC0C1u,
    0xC181u,
    0x0140u,
    0xC301u,
    0x03C0u,
    0x0280u,
    0xC241u,
    0xC601u,
    0x06C0u,
    0x0780u,
    0xC741u,
    0x0500u,
    0xC5C1u,
    0xC481u,
    0x0440u,
    0xCC01u,
    0x0CC0u,
    0x0D80u,
    0xCD41u,
    0x0F00u,
    0xCFC1u,
    0xCE81u,
    0x0E40u,
    0x0A00u,
    0xCAC1u,
    0xCB81u,
    0x0B40u,
    0xC901u,
    0x09C0u,
    0x0880u,
    0xC841u,
    0xD801u,
    0x18C0u,
    0x1980u,
    0xD941u,
    0x1B00u,
    0xDBC1u,
    0xDA81u,
    0x1A40u,
    0x1E00u,
    0xDEC1u,
    0xDF81u,
    0x1F40u,
    0xDD01u,
    0x1DC0u,
    0x1C80u,
    0xDC41u,
    0x1400u,
    0xD4C1u,
    0xD581u,
    0x1540u,
    0xD701u,
    0x17C0u,
    0x1680u,
    0xD641u,
    0xD201u,
    0x12C0u,
    0x1380u,
    0xD341u,
    0x1100u,
    0xD1C1u,
    0xD081u,
    0x1040u,
    0xF001u,
    0x30C0u,
    0x3180u,
    0xF141u,
    0x3300u,
    0xF3C1u,
    0xF281u,
    0x3240u,
    0x3600u,
    0xF6C1u,
    0xF781u,
    0x3740u,
    0xF501u,
    0x35C0u,
    0x3480u,
    0xF441u,
    0x3C00u,
    0xFCC1u,
    0xFD81u,
    0x3D40u,
    0xFF01u,
    0x3FC0u,
    0x3E80u,
    0xFE41u,
    0xFA01u,
    0x3AC0u,
    0x3B80u,
    0xFB41u,
    0x3900u,
    0xF9C1u,
    0xF881u,
    0x3840u,
    0x2800u,
    0xE8C1u,
    0xE981u,
    0x2940u,
    0xEB01u,
    0x2BC0u,
    0x2A80u,
    0xEA41u,
    0xEE01u,
    0x2EC0u,
    0x2F80u,
    0xEF41u,
    0x2D00u,
    0xEDC1u,
    0xEC81u,
    0x2C40u,
    0xE401u,
    0x24C0u,
    0x2580u,
    0xE541u,
    0x2700u,
    0xE7C1u,
    0xE681u,
    0x2640u,
    0x2200u,
    0xE2C1u,
    0xE381u,
    0x2340u,
    0xE101u,
    0x21C0u,
    0x2080u,
    0xE041u,
    0xA001u,
    0x60C0u,
    0x6180u,
    0xA141u,
    0x6300u,
    0xA3C1u,
    0xA281u,
    0x6240u,
    0x6600u,
    0xA6C1u,
    0xA781u,
    0x6740u,
    0xA501u,
    0x65C0u,
    0x6480u,
    0xA441u,
    0x6C00u,
    0xACC1u,
    0xAD81u,
    0x6D40u,
    0xAF01u,
    0x6FC0u,
    0x6E80u,
    0xAE41u,
    0xAA01u,
    0x6AC0u,
    0x6B80u,
    0xAB41u,
    0x6900u,
    0xA9C1u,
    0xA881u,
    0x6840u,
    0x7800u,
    0xB8C1u,
    0xB981u,
    0x7940u,
    0xBB01u,
    0x7BC0u,
    0x7A80u,
    0xBA41u,
    0xBE01u,
    0x7EC0u,
    0x7F80u,
    0xBF41u,
    0x7D00u,
    0xBDC1u,
    0xBC81u,
    0x7C40u,
    0xB401u,
    0x74C0u,
    0x7580u,
    0xB541u,
    0x7700u,
    0xB7C1u,
    0xB681u,
    0x7640u,
    0x7200u,
    0xB2C1u,
    0xB381u,
    0x7340u,
    0xB101u,
    0x71C0u,
    0x7080u,
    0xB041u,
    0x5000u,
    0x90C1u,
    0x9181u,
    0x5140u,
    0x9301u,
    0x53C0u,
    0x5280u,
    0x9241u,
    0x9601u,
    0x56C0u,
    0x5780u,
    0x9741u,
    0x5500u,
    0x95C1u,
    0x9481u,
    0x5440u,
    0x9C01u,
    0x5CC0u,
    0x5D80u,
    0x9D41u,
    0x5F00u,
    0x9FC1u,
    0x9E81u,
    0x5E40u,
    0x5A00u,
    0x9AC1u,
    0x9B81u,
    0x5B40u,
    0x9901u,
    0x59C0u,
    0x5880u,
    0x9841u,
    0x8801u,
    0x48C0u,
    0x4980u,
    0x8941u,
    0x4B00u,
    0x8BC1u,
    0x8A81u,
    0x4A40u,
    0x4E00u,
    0x8EC1u,
    0x8F81u,
    0x4F40u,
    0x8D01u,
    0x4DC0u,
    0x4C80u,
    0x8C41u,
    0x4400u,
    0x84C1u,
    0x8581u,
    0x4540u,
    0x8701u,
    0x47C0u,
    0x4680u,
    0x8641u,
    0x8201u,
    0x42C0u,
    0x4380u,
    0x8341u,
    0x4100u,
    0x81C1u,
    0x8081u,
    0x4040u};

/*
 * uint16_t crc_16( const unsigned char *input_str, size_t num_bytes );
 *
 * The function crc_16() calculates the 16 bits CRC16 in one pass for a byte
 * string of which the beginning has been passed to the function. The number of
 * bytes to check is also a parameter. The number of the bytes in the string is
 * limited by the constant SIZE_MAX.
 */

uint16_t crc_16(const unsigned char * input_str, size_t num_bytes) {
    uint16_t crc;
    const unsigned char * ptr;
    size_t a;

    crc = CRC_START_16;
    ptr = input_str;

    for (a = 0; a < num_bytes; a++) {
        crc = (uint16_t)((crc >> 8) ^ crc_tab16[(crc ^ (uint16_t)*ptr++) & 0x00FF]);
    }

    return crc;

} /* crc_16 */

/*
 * uint16_t crc_modbus( const unsigned char *input_str, size_t num_bytes );
 *
 * The function crc_modbus() calculates the 16 bits Modbus CRC in one pass for
 * a byte string of which the beginning has been passed to the function. The
 * number of bytes to check is also a parameter.
 */

uint16_t crc_modbus(const unsigned char * input_str, size_t num_bytes) {
    uint16_t crc;
    const unsigned char * ptr;
    size_t a;

    crc = CRC_START_MODBUS;
    ptr = input_str;

    for (a = 0; a < num_bytes; a++) {
        crc = (uint16_t)((crc >> 8) ^ crc_tab16[(crc ^ (uint16_t)*ptr++) & 0x00FF]);
    }

    return crc;

} /* crc_modbus */

/*
 * uint16_t update_crc_16( uint16_t crc, unsigned char c );
 *
 * The function update_crc_16() calculates a new CRC-16 value based on the
 * previous value of the CRC and the next byte of data to be checked.
 */

uint16_t update_crc_16(uint16_t crc, unsigned char c) {
    return (uint16_t)((crc >> 8) ^ crc_tab16[(crc ^ (uint16_t)c) & 0x00FF]);
} /* update_crc_16 */

/*
* static uint8_t sht75_crc_table[];
*
* The SHT75 humidity sensor is capable of calculating an 8 bit CRC checksum to
* ensure data integrity. The lookup table crc_table[] is used to recalculate
* the CRC.
*/

static uint8_t sht75_crc_table[] = {

    0,
    49,
    98,
    83,
    196,
    245,
    166,
    151,
    185,
    136,
    219,
    234,
    125,
    76,
    31,
    46,
    67,
    114,
    33,
    16,
    135,
    182,
    229,
    212,
    250,
    203,
    152,
    169,
    62,
    15,
    92,
    109,
    134,
    183,
    228,
    213,
    66,
    115,
    32,
    17,
    63,
    14,
    93,
    108,
    251,
    202,
    153,
    168,
    197,
    244,
    167,
    150,
    1,
    48,
    99,
    82,
    124,
    77,
    30,
    47,
    184,
    137,
    218,
    235,
    61,
    12,
    95,
    110,
    249,
    200,
    155,
    170,
    132,
    181,
    230,
    215,
    64,
    113,
    34,
    19,
    126,
    79,
    28,
    45,
    186,
    139,
    216,
    233,
    199,
    246,
    165,
    148,
    3,
    50,
    97,
    80,
    187,
    138,
    217,
    232,
    127,
    78,
    29,
    44,
    2,
    51,
    96,
    81,
    198,
    247,
    164,
    149,
    248,
    201,
    154,
    171,
    60,
    13,
    94,
    111,
    65,
    112,
    35,
    18,
    133,
    180,
    231,
    214,
    122,
    75,
    24,
    41,
    190,
    143,
    220,
    237,
    195,
    242,
    161,
    144,
    7,
    54,
    101,
    84,
    57,
    8,
    91,
    106,
    253,
    204,
    159,
    174,
    128,
    177,
    226,
    211,
    68,
    117,
    38,
    23,
    252,
    205,
    158,
    175,
    56,
    9,
    90,
    107,
    69,
    116,
    39,
    22,
    129,
    176,
    227,
    210,
    191,
    142,
    221,
    236,
    123,
    74,
    25,
    40,
    6,
    55,
    100,
    85,
    194,
    243,
    160,
    145,
    71,
    118,
    37,
    20,
    131,
    178,
    225,
    208,
    254,
    207,
    156,
    173,
    58,
    11,
    88,
    105,
    4,
    53,
    102,
    87,
    192,
    241,
    162,
    147,
    189,
    140,
    223,
    238,
    121,
    72,
    27,
    42,
    193,
    240,
    163,
    146,
    5,
    52,
    103,
    86,
    120,
    73,
    26,
    43,
    188,
    141,
    222,
    239,
    130,
    179,
    224,
    209,
    70,
    119,
    36,
    21,
    59,
    10,
    89,
    104,
    255,
    206,
    157,
    172};

/*
* uint8_t crc_8( const unsigned char *input_str, size_t num_bytes );
*
* The function crc_8() calculates the 8 bit wide CRC of an input string of a
* given length.
*/

uint8_t crc_8(const unsigned char * input_str, size_t num_bytes) {
    size_t a;
    uint8_t crc;
    const unsigned char * ptr;

    crc = CRC_START_8;
    ptr = input_str;

    for (a = 0; a < num_bytes; a++) {
        crc = sht75_crc_table[(*ptr++) ^ crc];
    }

    return crc;

} /* crc_8 */

/*
   * uint8_t update_crc_8( unsigned char crc, unsigned char val );
   *
   * Given a databyte and the previous value of the CRC value, the function
   * update_crc_8() calculates and returns the new actual CRC value of the data
   * comming in.
   */

uint8_t update_crc_8(uint8_t crc, unsigned char val) {
    return sht75_crc_table[val ^ crc];

} /* update_crc_8 */
