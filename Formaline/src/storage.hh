// $Header$

#ifndef FORMALINE_STORAGE_HH
#define FORMALINE_STORAGE_HH



namespace Formaline
{



  class storage
  {
  public:
  
    enum state { initial, update, final };
  
  private:
  
    enum state m_state;
  
  public:
  
    storage (enum state);
  
    virtual
    ~ storage ();
  
    enum state
    get_state ()
      const;
  
    virtual void
    store (char const * key,
           bool value)
      = 0;
  
    virtual void
    store (char const * key,
           int value)
      = 0;
  
    virtual void
    store (char const * key,
           double value)
      = 0;
  
    virtual void
    store (char const * key,
           char const * value)
      = 0;
  };



} // namespace Formaline



#endif // ifndef FORMALINE_STORAGE_HH
