#ifndef ACE_AUTO_EVENT_CPP
#define ACE_AUTO_EVENT_CPP
#include "ace/Auto_Event.h"

#if defined (ACE_HAS_ALLOC_HOOKS)
# include "ace/Malloc_Base.h"
#endif /* ACE_HAS_ALLOC_HOOKS */

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_ALLOC_HOOK_DEFINE_Tc(ACE_Auto_Event_T)

template <class TIME_POLICY>
ACE_Auto_Event_T<TIME_POLICY>::ACE_Auto_Event_T (
    int initial_state,
    int type,
    const char *name,
    void *arg)
  : ACE_Event_T<TIME_POLICY> (0,
                              initial_state,
                              type,
                              ACE_TEXT_CHAR_TO_TCHAR (name),
                              arg)
{
}

#if defined (ACE_HAS_WCHAR)
template <class TIME_POLICY>
ACE_Auto_Event_T<TIME_POLICY>::ACE_Auto_Event_T (
    int initial_state,
    int type,
    const wchar_t *name,
    void *arg)
  : ACE_Event_T<TIME_POLICY> (0,
                              initial_state,
                              type,
                              ACE_TEXT_WCHAR_TO_TCHAR (name),
                              arg)
{
}
#endif /* ACE_HAS_WCHAR */

template <class TIME_POLICY>
void
ACE_Auto_Event_T<TIME_POLICY>::dump () const
{
#if defined (ACE_HAS_DUMP)
  ACE_Event_T<TIME_POLICY>::dump ();
#endif /* ACE_HAS_DUMP */
}

ACE_END_VERSIONED_NAMESPACE_DECL

#endif
