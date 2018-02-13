// *****************************************************************************
// Filename    : Startup.c
// 
// Core        : Renesas RH850G3K 
// 
// Author      : Chalandi Amine
//
// Owner       : Chalandi Amine
// 
// Date        : 06.04.2017
// 
// Description : Crt0 for RH850G3K
//
// Compiler    : GHS
//
// Licence     : GNU General Public License v3.0
//
// *****************************************************************************

#pragma ghs section text=".startup"

//=============================================================================
// extern function prototypes
//=============================================================================
extern int main(void);

//=============================================================================
// Macros
//=============================================================================
#define __weak  __attribute__((weak))
#define CALL_WEAK_FUNC(function) if(function) function##();

//=============================================================================
// function prototypes
//=============================================================================
__weak void InitSysClocks(void);

//=============================================================================
// types definitions
//=============================================================================
typedef struct _runtimeCopyTable_t {

    unsigned int            targetAddr;  /* Target Address (section in RAM memory) */
    unsigned int            sourceAddr;  /* Source Address (section in ROM memory) */
    unsigned int            size;        /* length of section (bytes) */

} runtimeCopyTable_t;

//=============================================================================
// linker variables
//=============================================================================
#pragma ghs startdata /* do not use TP and GP registers to access to those variables */
extern const runtimeCopyTable_t __ghsbegin_secinfo[];
extern unsigned int __ghssize_secinfo;
extern unsigned int __ghsbinfo_clear;
extern unsigned int __ghseinfo_clear;
extern unsigned int __ghsbinfo_copy;
extern unsigned int __ghseinfo_copy;
extern unsigned int __ghsbegin_stack;
extern unsigned int __ghsend_stack;
#pragma ghs enddata

//-----------------------------------------------------------------------------
/// \brief  This function clear the stack just befor calling the main function 
///         and set the return address to the address of the function 
///         SysStartup_UnexpectedExitFromMain.
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------
asm void SysStartup_CallMainFunction(void) 
{
  -- Init stack
  mov ___ghsbegin_stack,r20
  mov ___ghssize_stack,r21
  mov 0xAAAAAAAA,r22

again:
  st.w r22,0[r20]
  add 4,r20
  add -4,r21
  cmp r21,r0
  bne again

  -- Init stack pointer
  mov (___ghsend_stack), sp

  -- align the stack to a 4-byte boundary

  mov -4, r1
  and r1, sp
  
  -- Jump to main
  mov _SysStartup_UnexpectedExitFromMain,lp
  mov _main,r1
  jmp r1
}

//-----------------------------------------------------------------------------
/// \brief  This function init the C runtime environement by setting the GP,
///         LP, SP, INTP and RBASE registers.
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------
asm void SysStartup_RuntimeEnvironment(void)
{

  .weak  ___ghsbegin_sda_start
  .weak  ___ghsbegin_sda_end
  .weak  ___ghsbegin_rosda_start
  .weak  ___ghsbegin_rosda_end

  
  mov (__gp), gp
  mov (__tp), tp  
  
  -- Initialize the stack pointer (to the top of .stack section).
  
  mov (___ghsend_stack), sp

  -- align the stack to a 4-byte boundary

  mov -4, r1
  and r1, sp
  
  -- Initialization of the interrupt base pointer

  --mov  _IBP_ADDR,r1
  --ldsr r1,intbp,1

  -- Set RBASE register address

  mov  ___ghsbegin_intvect, r10
  ldsr r10, RBASE, 1
}
//-----------------------------------------------------------------------------
/// \brief  Memset function
///
/// \param  unsigned char* ptr: pointer to the target memory
///                 int value : initliaziation value 
///         unsigned int size : size of the memory region to set
///
/// \return void
//-----------------------------------------------------------------------------
static void SysStartup_Memset(unsigned char* ptr,int value, unsigned int size)
{
  for(int i=0;i<size;i++)
  {
    *((unsigned char*)(ptr+i)) = value;
  }
}
//-----------------------------------------------------------------------------
/// \brief  Memcpy function
///
/// \param         unsigned char* target: pointer to the target memory region
///         const unsigned char* source : pointer to the source memory region
///                   unsigned int size : size of the memory region to copy
///
/// \return void
//-----------------------------------------------------------------------------
static void SysStartup_Memcpy(unsigned char* target, const unsigned char* source, unsigned int size)
{
  for(int i=0;i<size;i++)
  {
    *((unsigned char*)(target+i)) = *((const unsigned char*)(source+i));
  }
}

//-----------------------------------------------------------------------------
/// \brief  This function initalize the RAM by getting informations about
///         runtime copy and clear table from parsing the .secinfo section 
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------
static void SysStartup_InitMemory(void)
{
  unsigned int const size = ((unsigned int)&__ghssize_secinfo)/sizeof(runtimeCopyTable_t);

  for(int i=0; i<size;i++)
  {
    /* CLEAR TABLE */ 
    if((unsigned int)&__ghsbegin_secinfo[i] >= (unsigned int)&__ghsbinfo_clear && 
       (unsigned int)&__ghsbegin_secinfo[i] < (unsigned int)&__ghseinfo_clear  
      )
    {
      /* Clear Memory Area */
      if(((unsigned int)__ghsbegin_secinfo[i].targetAddr < (unsigned int)&__ghsbegin_stack) || 
         ((unsigned int)__ghsbegin_secinfo[i].targetAddr > (unsigned int)&__ghsend_stack) //to avoid clearing the stack during runtime execution
        )
        {
          SysStartup_Memset((unsigned char*)__ghsbegin_secinfo[i].targetAddr,0,__ghsbegin_secinfo[i].size);
        }
    }

    /* COPY TABLE */ 
    if((unsigned int)&__ghsbegin_secinfo[i] >= (unsigned int)&__ghsbinfo_copy && 
       (unsigned int)&__ghsbegin_secinfo[i] < (unsigned int)&__ghseinfo_copy
      )
    {
      /* Copy Memory Area */
      SysStartup_Memcpy((unsigned char*)__ghsbegin_secinfo[i].targetAddr,(const unsigned char*)__ghsbegin_secinfo[i].sourceAddr,__ghsbegin_secinfo[i].size);
    }
  }
}
//-----------------------------------------------------------------------------
/// \brief  This function is the main function of the startup
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------
#pragma ghs noprologue
void SysStartup(void)
{
  // Initial C runtime Environment
  SysStartup_RuntimeEnvironment();

  //Init system clocks
  CALL_WEAK_FUNC(InitSysClocks);
  
  //Runtime Clear and Copy table
  SysStartup_InitMemory();

  //Jump to the main.
  SysStartup_CallMainFunction();
}

//-----------------------------------------------------------------------------
/// \brief  This function catch unexpected exit from the main function.
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------
void SysStartup_UnexpectedExitFromMain(void)
{
  for(;;);
}
