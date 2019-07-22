/* EVMC: Ethereum Client-VM Connector API.
 * Copyright 2018-2019 The EVMC Authors.
 * Licensed under the Apache License, Version 2.0.
 */
#pragma once

#include <evmc/evmc.h>
#include <evmc/helpers.h>

#include <functional>
#include <initializer_list>
#include <utility>

/// EVMC C++ API - wrappers and bindings for C++
/// @ingroup cpp
namespace evmc
{
/// The big-endian 160-bit hash suitable for keeping an Ethereum address.
///
/// This type wraps C ::evmc_address to make sure objects of this type are always initialized.
struct address : evmc_address
{
    /// Default and converting constructor.
    ///
    /// Initializes bytes to zeros if not other @p init value provided.
    constexpr address(evmc_address init = {}) noexcept : evmc_address{init} {}

    /// Explicit operator converting to bool.
    constexpr inline explicit operator bool() const noexcept;
};

/// The fixed size array of 32 bytes for storing 256-bit EVM values.
///
/// This type wraps C ::evmc_bytes32 to make sure objects of this type are always initialized.
struct bytes32 : evmc_bytes32
{
    /// Default and converting constructor.
    ///
    /// Initializes bytes to zeros if not other @p init value provided.
    constexpr bytes32(evmc_bytes32 init = {}) noexcept : evmc_bytes32{init} {}

    /// Explicit operator converting to bool.
    constexpr inline explicit operator bool() const noexcept;
};

/// Loads 64 bits / 8 bytes of data from the given @p bytes array in big-endian order.
constexpr inline uint64_t load64be(const uint8_t* bytes) noexcept
{
    // TODO: Report bug in clang incorrectly optimizing this with AVX2 enabled.
    return (uint64_t{bytes[0]} << 56) | (uint64_t{bytes[1]} << 48) | (uint64_t{bytes[2]} << 40) |
           (uint64_t{bytes[3]} << 32) | (uint64_t{bytes[4]} << 24) | (uint64_t{bytes[5]} << 16) |
           (uint64_t{bytes[6]} << 8) | uint64_t{bytes[7]};
}

/// Loads 32 bits / 4 bytes of data from the given @p bytes array in big-endian order.
constexpr inline uint32_t load32be(const uint8_t* bytes) noexcept
{
    return (uint32_t{bytes[0]} << 24) | (uint32_t{bytes[1]} << 16) | (uint32_t{bytes[2]} << 8) |
           uint32_t{bytes[3]};
}

namespace fnv
{
constexpr auto prime = 0x100000001b3;              ///< The 64-bit FNV prime number.
constexpr auto offset_basis = 0xcbf29ce484222325;  ///< The 64-bit FNV offset basis.

/// The hashing transformation for 64-bit inputs based on the FNV-1a formula.
constexpr inline uint64_t fnv1a_by64(uint64_t h, uint64_t x) noexcept
{
    return (h ^ x) * prime;
}
}  // namespace fnv


/// The "equal" comparison operator for the evmc::address type.
constexpr bool operator==(const address& a, const address& b) noexcept
{
    // TODO: Report bug in clang keeping unnecessary bswap.
    return load64be(&a.bytes[0]) == load64be(&b.bytes[0]) &&
           load64be(&a.bytes[8]) == load64be(&b.bytes[8]) &&
           load32be(&a.bytes[16]) == load32be(&b.bytes[16]);
}

/// The "not equal" comparison operator for the evmc::address type.
constexpr bool operator!=(const address& a, const address& b) noexcept
{
    return !(a == b);
}

/// The "less" comparison operator for the evmc::address type.
constexpr bool operator<(const address& a, const address& b) noexcept
{
    return load64be(&a.bytes[0]) < load64be(&b.bytes[0]) ||
           (load64be(&a.bytes[0]) == load64be(&b.bytes[0]) &&
            load64be(&a.bytes[8]) < load64be(&b.bytes[8])) ||
           (load64be(&a.bytes[8]) == load64be(&b.bytes[8]) &&
            load32be(&a.bytes[16]) < load32be(&b.bytes[16]));
}

/// The "equal" comparison operator for the evmc::bytes32 type.
constexpr bool operator==(const bytes32& a, const bytes32& b) noexcept
{
    return load64be(&a.bytes[0]) == load64be(&b.bytes[0]) &&
           load64be(&a.bytes[8]) == load64be(&b.bytes[8]) &&
           load64be(&a.bytes[16]) == load64be(&b.bytes[16]) &&
           load64be(&a.bytes[24]) == load64be(&b.bytes[24]);
}

/// The "not equal" comparison operator for the evmc::bytes32 type.
constexpr bool operator!=(const bytes32& a, const bytes32& b) noexcept
{
    return !(a == b);
}

/// The "less" comparison operator for the evmc::bytes32 type.
constexpr bool operator<(const bytes32& a, const bytes32& b) noexcept
{
    return load64be(&a.bytes[0]) < load64be(&b.bytes[0]) ||
           (load64be(&a.bytes[0]) == load64be(&b.bytes[0]) &&
            load64be(&a.bytes[8]) < load64be(&b.bytes[8])) ||
           (load64be(&a.bytes[8]) == load64be(&b.bytes[8]) &&
            load64be(&a.bytes[16]) < load64be(&b.bytes[16])) ||
           (load64be(&a.bytes[16]) == load64be(&b.bytes[16]) &&
            load64be(&a.bytes[24]) < load64be(&b.bytes[24]));
}

/// Checks if the given address is the zero address.
constexpr inline bool is_zero(const address& a) noexcept
{
    return a == address{};
}

constexpr address::operator bool() const noexcept
{
    return !is_zero(*this);
}

/// Checks if the given bytes32 object has all zero bytes.
constexpr inline bool is_zero(const bytes32& a) noexcept
{
    return a == bytes32{};
}

constexpr bytes32::operator bool() const noexcept
{
    return !is_zero(*this);
}


/// @copydoc evmc_result
///
/// This is a RAII wrapper for evmc_result and objects of this type
/// automatically release attached resources.
class result : private evmc_result
{
public:
    using evmc_result::create_address;
    using evmc_result::gas_left;
    using evmc_result::output_data;
    using evmc_result::output_size;
    using evmc_result::status_code;

    /// Converting constructor from raw evmc_result.
    explicit result(evmc_result const& res) noexcept : evmc_result{res} {}

    /// Destructor responsible for automatically releasing attached resources.
    ~result() noexcept
    {
        if (release)
            release(this);
    }

    /// Move constructor.
    result(result&& other) noexcept : evmc_result{other}
    {
        other.release = nullptr;  // Disable releasing of the rvalue object.
    }

    /// Move assignment operator.
    ///
    /// The self-assigment MUST never happen.
    ///
    /// @param other The other result object.
    /// @return      The reference to the left-hand side object.
    result& operator=(result&& other) noexcept
    {
        this->~result();                           // Release this object.
        static_cast<evmc_result&>(*this) = other;  // Copy data.
        other.release = nullptr;                   // Disable releasing of the rvalue object.
        return *this;
    }

    /// Releases the ownership and returns the raw copy of evmc_result.
    ///
    /// This method drops the ownership of the result
    /// (result's resources are not going to be released when this object is destructed).
    /// It is the caller's responsibility having the returned copy of the result to release it.
    /// This object MUST NOT be used after this method is invoked.
    ///
    /// @return  The copy of this object converted to raw evmc_result.
    evmc_result release_raw() noexcept
    {
        const auto out = evmc_result{*this};  // Copy data.
        this->release = nullptr;              // Disable releasing of this object.
        return out;
    }
};

/// @copybrief evmc_instance
///
/// This is a RAII wrapper for evmc_instance and objects of this type
/// automatically destroys the VM instance.
class vm
{
public:
    vm() noexcept = default;

    /// Converting constructor from evmc_instance.
    explicit vm(evmc_instance* instance) noexcept : m_instance{instance} {}

    /// Destructor responsible for automatically destroying the VM instance.
    ~vm() noexcept
    {
        if (m_instance)
            m_instance->destroy(m_instance);
    }

    vm(const vm&) = delete;
    vm& operator=(const vm&) = delete;

    /// Move constructor.
    vm(vm&& other) noexcept : m_instance{other.m_instance} { other.m_instance = nullptr; }

    /// Move assignment operator.
    vm& operator=(vm&& other) noexcept
    {
        this->~vm();
        m_instance = other.m_instance;
        other.m_instance = nullptr;
        return *this;
    }

    /// The constructor that captures a VM instance and configures the instance
    /// with provided list of options.
    vm(evmc_instance* instance,
       std::initializer_list<std::pair<const char*, const char*>> options) noexcept
      : m_instance{instance}
    {
        for (auto option : options)
            set_option(option.first, option.second);
    }

    /// Checks if contains a valid pointer to the VM instance.
    explicit operator bool() const noexcept { return m_instance != nullptr; }

    /// Checks whenever the VM instance is ABI compatible with the current EVMC API.
    bool is_abi_compatible() const noexcept { return m_instance->abi_version == EVMC_ABI_VERSION; }

    /// @copydoc evmc_instance::name
    char const* name() const noexcept { return m_instance->name; }

    /// @copydoc evmc_instance::version
    char const* version() const noexcept { return m_instance->version; }

    /// @copydoc evmc::instance::get_capabilities
    evmc_capabilities_flagset get_capabilities() const noexcept
    {
        return m_instance->get_capabilities(m_instance);
    }

    /// @copydoc evmc_set_option()
    evmc_set_option_result set_option(const char name[], const char value[]) noexcept
    {
        return evmc_set_option(m_instance, name, value);
    }

    /// @copydoc evmc_execute()
    result execute(evmc_context& ctx,
                   evmc_revision rev,
                   const evmc_message& msg,
                   const uint8_t* code,
                   size_t code_size) noexcept
    {
        return result{m_instance->execute(m_instance, &ctx, rev, &msg, code, code_size)};
    }

private:
    evmc_instance* m_instance = nullptr;
};

/// The EVMC Host interface
class HostInterface
{
public:
    virtual ~HostInterface() noexcept = default;

    /// @copydoc evmc_host_interface::account_exists
    virtual bool account_exists(const evmc_address& addr) noexcept = 0;

    /// @copydoc evmc_host_interface::get_storage
    virtual evmc_bytes32 get_storage(const evmc_address& addr,
                                     const evmc_bytes32& key) noexcept = 0;

    /// @copydoc evmc_host_interface::set_storage
    virtual evmc_storage_status set_storage(const evmc_address& addr,
                                            const evmc_bytes32& key,
                                            const evmc_bytes32& value) noexcept = 0;

    /// @copydoc evmc_host_interface::get_balance
    virtual evmc_uint256be get_balance(const evmc_address& addr) noexcept = 0;

    /// @copydoc evmc_host_interface::get_code_size
    virtual size_t get_code_size(const evmc_address& addr) noexcept = 0;

    /// @copydoc evmc_host_interface::get_code_hash
    virtual evmc_bytes32 get_code_hash(const evmc_address& addr) noexcept = 0;

    /// @copydoc evmc_host_interface::copy_code
    virtual size_t copy_code(const evmc_address& addr,
                             size_t code_offset,
                             uint8_t* buffer_data,
                             size_t buffer_size) noexcept = 0;

    /// @copydoc evmc_host_interface::selfdestruct
    virtual void selfdestruct(const evmc_address& addr,
                              const evmc_address& beneficiary) noexcept = 0;

    /// @copydoc evmc_host_interface::call
    virtual result call(const evmc_message& msg) noexcept = 0;

    /// @copydoc evmc_host_interface::get_tx_context
    virtual evmc_tx_context get_tx_context() noexcept = 0;

    /// @copydoc evmc_host_interface::get_block_hash
    virtual evmc_bytes32 get_block_hash(int64_t block_number) noexcept = 0;

    /// @copydoc evmc_host_interface::emit_log
    virtual void emit_log(const evmc_address& addr,
                          const uint8_t* data,
                          size_t data_size,
                          const evmc_bytes32 topics[],
                          size_t num_topics) noexcept = 0;
};


/// Wrapper around EVMC host context / host interface.
///
/// To be used by VM implementations as better alternative to using ::evmc_context directly.
class HostContext : public HostInterface
{
    evmc_context* context = nullptr;
    evmc_tx_context tx_context = {};

public:
    /// Implicit converting constructor from evmc_context.
    HostContext(evmc_context* ctx) noexcept : context{ctx} {}  // NOLINT

    bool account_exists(const evmc_address& address) noexcept final
    {
        return context->host->account_exists(context, &address);
    }

    evmc_bytes32 get_storage(const evmc_address& address, const evmc_bytes32& key) noexcept final
    {
        return context->host->get_storage(context, &address, &key);
    }

    evmc_storage_status set_storage(const evmc_address& address,
                                    const evmc_bytes32& key,
                                    const evmc_bytes32& value) noexcept final
    {
        return context->host->set_storage(context, &address, &key, &value);
    }

    evmc_uint256be get_balance(const evmc_address& address) noexcept final
    {
        return context->host->get_balance(context, &address);
    }

    size_t get_code_size(const evmc_address& address) noexcept final
    {
        return context->host->get_code_size(context, &address);
    }

    evmc_bytes32 get_code_hash(const evmc_address& address) noexcept final
    {
        return context->host->get_code_hash(context, &address);
    }

    size_t copy_code(const evmc_address& address,
                     size_t code_offset,
                     uint8_t* buffer_data,
                     size_t buffer_size) noexcept final
    {
        return context->host->copy_code(context, &address, code_offset, buffer_data, buffer_size);
    }

    void selfdestruct(const evmc_address& address, const evmc_address& beneficiary) noexcept final
    {
        context->host->selfdestruct(context, &address, &beneficiary);
    }

    result call(const evmc_message& message) noexcept final
    {
        return result{context->host->call(context, &message)};
    }

    /// @copydoc HostInterface::get_tx_context()
    ///
    /// The implementation caches the received transaction context
    /// by assuming that the block timestamp should never be zero.
    ///
    /// @return The cached transaction context.
    evmc_tx_context get_tx_context() noexcept final
    {
        if (tx_context.block_timestamp == 0)
            tx_context = context->host->get_tx_context(context);
        return tx_context;
    }

    evmc_bytes32 get_block_hash(int64_t number) noexcept final
    {
        return context->host->get_block_hash(context, number);
    }

    void emit_log(const evmc_address& address,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc_bytes32 topics[],
                  size_t topics_count) noexcept final
    {
        context->host->emit_log(context, &address, data, data_size, topics, topics_count);
    }
};

/// Abstract class to be used by Host implementations.
///
/// When implementing EVMC Host, you can directly inherit from the evmc::Host class.
/// This way your implementation will be simpler by avoiding manual handling
/// of the ::evmc_context and the ::evmc_context::host.
class Host : public HostInterface, public evmc_context
{
public:
    inline Host() noexcept;
};

namespace internal
{
inline bool account_exists(evmc_context* h, const evmc_address* addr) noexcept
{
    return static_cast<Host*>(h)->account_exists(*addr);
}
inline evmc_bytes32 get_storage(evmc_context* h,
                                const evmc_address* addr,
                                const evmc_bytes32* key) noexcept
{
    return static_cast<Host*>(h)->get_storage(*addr, *key);
}
inline evmc_storage_status set_storage(evmc_context* h,
                                       const evmc_address* addr,
                                       const evmc_bytes32* key,
                                       const evmc_bytes32* value) noexcept
{
    return static_cast<Host*>(h)->set_storage(*addr, *key, *value);
}
inline evmc_uint256be get_balance(evmc_context* h, const evmc_address* addr) noexcept
{
    return static_cast<Host*>(h)->get_balance(*addr);
}
inline size_t get_code_size(evmc_context* h, const evmc_address* addr) noexcept
{
    return static_cast<Host*>(h)->get_code_size(*addr);
}
inline evmc_bytes32 get_code_hash(evmc_context* h, const evmc_address* addr) noexcept
{
    return static_cast<Host*>(h)->get_code_hash(*addr);
}
inline size_t copy_code(evmc_context* h,
                        const evmc_address* addr,
                        size_t code_offset,
                        uint8_t* buffer_data,
                        size_t buffer_size) noexcept
{
    return static_cast<Host*>(h)->copy_code(*addr, code_offset, buffer_data, buffer_size);
}
inline void selfdestruct(evmc_context* h,
                         const evmc_address* addr,
                         const evmc_address* beneficiary) noexcept
{
    static_cast<Host*>(h)->selfdestruct(*addr, *beneficiary);
}
inline evmc_result call(evmc_context* h, const evmc_message* msg) noexcept
{
    return static_cast<Host*>(h)->call(*msg).release_raw();
}
inline evmc_tx_context get_tx_context(evmc_context* h) noexcept
{
    return static_cast<Host*>(h)->get_tx_context();
}
inline evmc_bytes32 get_block_hash(evmc_context* h, int64_t block_number) noexcept
{
    return static_cast<Host*>(h)->get_block_hash(block_number);
}
inline void emit_log(evmc_context* h,
                     const evmc_address* addr,
                     const uint8_t* data,
                     size_t data_size,
                     const evmc_bytes32 topics[],
                     size_t num_topics) noexcept
{
    static_cast<Host*>(h)->emit_log(*addr, data, data_size, topics, num_topics);
}

constexpr evmc_host_interface interface{
    account_exists, get_storage,  set_storage, get_balance,    get_code_size,  get_code_hash,
    copy_code,      selfdestruct, call,        get_tx_context, get_block_hash, emit_log,
};
}  // namespace internal

inline Host::Host() noexcept : evmc_context{&internal::interface} {}

}  // namespace evmc


namespace std
{
/// Hash operator template specialization for evmc::address. Needed for unordered containers.
template <>
struct hash<evmc::address>
{
    /// Hash operator using FNV1a-based folding.
    constexpr size_t operator()(const evmc::address& s) const noexcept
    {
        using namespace evmc;
        using namespace fnv;
        return static_cast<size_t>(fnv1a_by64(
            fnv1a_by64(fnv1a_by64(fnv::offset_basis, load64be(&s.bytes[0])), load64be(&s.bytes[8])),
            load32be(&s.bytes[16])));
    }
};

/// Hash operator template specialization for evmc::bytes32. Needed for unordered containers.
template <>
struct hash<evmc::bytes32>
{
    /// Hash operator using FNV1a-based folding.
    constexpr size_t operator()(const evmc::bytes32& s) const noexcept
    {
        using namespace evmc;
        using namespace fnv;
        return static_cast<size_t>(
            fnv1a_by64(fnv1a_by64(fnv1a_by64(fnv1a_by64(fnv::offset_basis, load64be(&s.bytes[0])),
                                             load64be(&s.bytes[8])),
                                  load64be(&s.bytes[16])),
                       load64be(&s.bytes[24])));
    }
};
}  // namespace std
