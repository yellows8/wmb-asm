#ifndef _DLL_H_
#define _DLL_H_

#ifdef BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#endif
#ifndef BUILDING_DLL /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */


class DLLIMPORT DllClass
{
  public:
    DllClass();
    virtual ~DllClass(void);

  private:

};


#endif /* _DLL_H_ */
