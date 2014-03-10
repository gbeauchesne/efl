#ifndef EFL_EINA_LOG_HH
#define EFL_EINA_LOG_HH

#include <sstream>

namespace efl { namespace eina {

namespace log_level {

struct critical_t { static constexpr ::Eina_Log_Level value = ::EINA_LOG_LEVEL_CRITICAL; };
critical_t const critical = {};

struct error_t { static constexpr ::Eina_Log_Level value = ::EINA_LOG_LEVEL_ERR; };
error_t const error = {};

struct info_t { static constexpr ::Eina_Log_Level value = ::EINA_LOG_LEVEL_INFO; };
info_t const info = {};

struct debug_t { static constexpr ::Eina_Log_Level value = ::EINA_LOG_LEVEL_DBG; };
debug_t const debug = {};

struct warn_t { static constexpr ::Eina_Log_Level value = ::EINA_LOG_LEVEL_WARN; };
warn_t const warning = {};

}

template <typename D>
struct _domain_base
{
  void set_level(log_level::critical_t l) { set_level(l.value); }
  void set_level(log_level::error_t l) { set_level(l.value); }
  void set_level(log_level::info_t l) { set_level(l.value); }
  void set_level(log_level::debug_t l) { set_level(l.value); }
  void set_level(log_level::warn_t l) { set_level(l.value); }
  void set_level( ::Eina_Log_Level l)
  {
    ::eina_log_domain_registered_level_set(static_cast<D&>(*this).domain_raw(), l);
  }
  ::Eina_Log_Level get_level() const
  {
    return static_cast< ::Eina_Log_Level>
      (::eina_log_domain_registered_level_get(static_cast<D const&>(*this).domain_raw()));
  }
};

const struct global_domain : _domain_base<global_domain>
{
  int domain_raw() const { return EINA_LOG_DOMAIN_GLOBAL; }
} global_domain;

const struct default_domain : _domain_base<default_domain>
{
  int domain_raw() const { return EINA_LOG_DOMAIN_DEFAULT; }
} default_domain;

struct log_domain : _domain_base<log_domain>
{
  log_domain(char const* name, char const* color = "black")
    : _domain( ::eina_log_domain_register(name, color))
  {
  }
  ~log_domain()
  {
    ::eina_log_domain_unregister(_domain);
  }
  int domain_raw() const { return _domain; }
private:
  int _domain;
};

inline void _log(std::stringstream const& stream, int domain, ::Eina_Log_Level level
                 , const char* file, const char* function, int line)
{
  ::eina_log_print(domain, level, file, function, line
                   , "%s", stream.str().c_str());
}

#define EINA_CXX_DOM_LOG(DOMAIN, LEVEL, EXPR)                           \
    if( ::eina_log_domain_level_check((DOMAIN), LEVEL))                 \
      {                                                                 \
        std::stringstream stream;                                       \
        stream << EXPR;                                                 \
        ::efl::eina::_log(std::move(stream), (DOMAIN), LEVEL            \
                          , __FILE__, __FUNCTION__, __LINE__);          \
      }

#define EINA_CXX_DOM_LOG_CRIT(DOMAIN, EXPR)                     \
    EINA_CXX_DOM_LOG(DOMAIN.domain_raw(), ::EINA_LOG_LEVEL_CRITICAL, EXPR)

#define EINA_CXX_DOM_LOG_ERR(DOMAIN, EXPR)                      \
    EINA_CXX_DOM_LOG(DOMAIN.domain_raw(), ::EINA_LOG_LEVEL_ERR, EXPR)

#define EINA_CXX_DOM_LOG_INFO(DOMAIN, EXPR)                      \
    EINA_CXX_DOM_LOG(DOMAIN.domain_raw(), ::EINA_LOG_LEVEL_INFO, EXPR)

#define EINA_CXX_DOM_LOG_DBG(DOMAIN, EXPR)                      \
    EINA_CXX_DOM_LOG(DOMAIN.domain_raw(), ::EINA_LOG_LEVEL_DBG, EXPR)

#define EINA_CXX_DOM_LOG_WARN(DOMAIN, EXPR)                      \
    EINA_CXX_DOM_LOG(DOMAIN.domain_raw(), ::EINA_LOG_LEVEL_WARN, EXPR)

#define EINA_CXX_LOG(LEVEL, EXPR)                               \
    EINA_CXX_DOM_LOG(EINA_LOG_DOMAIN_DEFAULT, LEVEL, EXPR)

#define EINA_CXX_LOG_CRIT(EXPR)                 \
    EINA_CXX_LOG(EINA_LOG_LEVEL_CRITICAL, EXPR)

#define EINA_CXX_LOG_ERR(EXPR)                  \
    EINA_CXX_LOG(EINA_LOG_LEVEL_ERR, EXPR)

#define EINA_CXX_LOG_INFO(EXPR)                 \
    EINA_CXX_LOG(EINA_LOG_LEVEL_INFO, EXPR)

#define EINA_CXX_LOG_DBG(EXPR)                  \
    EINA_CXX_LOG(EINA_LOG_LEVEL_DBG, EXPR)

#define EINA_CXX_LOG_WARN(EXPR)                 \
    EINA_CXX_LOG(EINA_LOG_LEVEL_WARN, EXPR)

} }

#endif