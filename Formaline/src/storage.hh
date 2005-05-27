// $Header$



#ifndef STORAGE_HH
#define STORAGE_HH



class storage
{
public:
  
  virtual
  ~ storage ();
  
  virtual void
  store (char const * key,
         int value);
  
  virtual void
  store (char const * key,
         char const * value);
  
protected:
  
  virtual void
  write (std::string const & msg)
    = 0;
  
  virtual std::string
  clean (std::string const & txt)
    const;
  
};



#endif // ifndef STORAGE_HH
