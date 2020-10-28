## Coding Style
### Overview
This Coding Style is for olsr program, follows the GNU coding standard layout for C, refers to the code style of crazyfile firmware and ns-3 format.
### Code layout
*  Indentation spacing is 2 spaces
*  Each statement should be put on a separate line to increase readability

``` 
void foo(void)
{
  if(condition)
    {
      
    }
}
```

### Name encoding
* Global variables should be prefixed with a “g_” and member variables (including static member variables) should be prefixed with a “m_”. 
* Function, method, and type names should follow the CamelCase convention: Words are joined without spaces and are capitalized,and lowercase first letter.
* Defined types will start with an upper case letter, consist of upper and lower case letters, and may optionally end with a “_t”.
* Defined constants will be all uppercase letters or numeric digits, with an underscore character separating words
* A long descriptive name which requires a lot of typing is always better than a short name which is hard to decipher.
```
#define MY_COMPUTER = 10;
typedef int newTypeOfInt_t;
int g_myComputer = 10;
static g_staticMyComputer = 20;
struct student { 
/*
*space between student and '{'
* '{' do not  be put on a separate line in struct defination.
*/ 
int m_id;
char m_myStudentName[10];
};
```

### File layout
* avoid multiple header includes
```
lpsTwrTag.h
#ifdef __LPS_TWR_TAG_H__
#define __LPS_TWR_TAG_H__
#endif
```

### Language features
* Variable declaration should have a short, one or two line comment describing the purpose of the variable, unless it is a local variable whose use is obvious from the context. The short comment should be on the same line as the variable declaration, unless it is too long, in which case it should be on the preceding lines.
* Every function should be preceded by a detailed (Doxygen) comment block describing what the function does, what the formal parameters are, and what the return value is (if any). Either Doxygen style “\” or “@”
  
```
int g_ansn = 0; //In order to distinguish tc message changes.

/**
  *@brief 函数简介
  *
  *@param 形参 参数说明
  *@param 形参 参数说明
  *@return 返回说明
  *  @retval 返回值说明
  *@note 注解
  *@attention 注意
  *@warning 警告
  *@exception 异常
*/
int  
```


