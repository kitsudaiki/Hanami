#ifndef BIT_BUFFER_H
#define BIT_BUFFER_H

#include <stdint.h>

#include <array>
#include <cstring>

template <uint64_t numberOfBits>
class BitBuffer
{
   public:
    BitBuffer() { clear(); }

    /**
     * @brief set buffer to zero
     */
    void clear()
    {
        std::memset(m_data.data(), 0, m_data.size());
        m_counter = 0;
    }

    /**
     * @brief set the bit at a specific position to a desired value
     *
     * @param pos position, which should be set
     * @param val value to set
     */
    void set(const uint64_t pos, const bool val)
    {
        if (pos >= m_numberOfBits) {
            return;
        }

        const uint64_t blockId = pos / 8;
        const uint8_t block = m_data[blockId];
        const uint8_t cmp = 1 << (pos % 8);

        if ((block & cmp) == 0 && val) {
            m_data[blockId] |= cmp;
            m_counter++;
        }
        if ((block & cmp) != 0 && val == false) {
            m_data[blockId] ^= cmp;
            m_counter--;
        }
    }

    /**
     * @brief get bit of a specific position
     *
     * @param pos position, where the bit should be checked
     *
     * @return true, if bit was set, else false
     */
    bool get(const uint64_t pos) const
    {
        if (pos >= m_numberOfBits) {
            return false;
        }

        const uint64_t blockId = pos / 8;
        const uint8_t block = m_data[blockId];
        const uint8_t cmp = 1 << (pos % 8);

        return (block & cmp) != 0;
    }

    /**
     * @brief check fi all bits in the buffer were set
     *
     * @return true, if all bits are set to 1, else false
     */
    bool isComplete() { return m_numberOfBits == m_counter; }

   private:
    std::array<uint8_t, (numberOfBits / 8) + 1> m_data;
    const uint64_t m_numberOfBits = numberOfBits;
    uint64_t m_counter = 0;
};

#endif  // BIT_BUFFER_H
