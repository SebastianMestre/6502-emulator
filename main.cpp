#include <array>

#include <cstdint>

using Word = uint8_t;

struct CPU {
	uint16_t PC; // Program Counter
	Word AC;     // Accumulator
	Word X;      // X register
	Word Y;      // Y register
	Word SR;     // Status Register (flags)
	Word SP;     // Stack Pointer
private:

	struct Flag {
		static constexpr int Negative  = 0x80;
		static constexpr int Overflow  = 0x40;
		static constexpr int Ignored   = 0x20;
		static constexpr int Break     = 0x10;
		static constexpr int Decimal   = 0x08;
		static constexpr int Interrupt = 0x04;
		static constexpr int Zero      = 0x02;
		static constexpr int Carry     = 0x01;
	};

	void clear_flags(Word flags) { SR &= ~flags; }
	void set_flags(Word flags) { SR |= flags; }
	void assign_flags(Word flags, bool value) {
		SR &= ~flags;
		SR |= value ? flags : 0;
	}

	void update_zero_flag(Word w) {
		assign_flags(Flag::Zero, w == 0);
	}

	void update_negative_flag(Word w) {
		assign_flags(Flag::Negative, w & (1 << 7));
	}

	void decrement(Word& w) {
		w -= 1;
		update_zero_flag(w);
		update_negative_flag(w);
	}

	void increment(Word &w) {
		w += 1;
		update_zero_flag(w);
		update_negative_flag(w);
	}

	void transfer(Word& dst, Word src) {
		dst = src;
		update_zero_flag(AC);
		update_negative_flag(AC);
	}

public:

	void nop() {}

	void eor(Word w) {
		AC ^= w;
		update_zero_flag(AC);
		update_negative_flag(AC);
	}

	void ora(Word w) {
		AC |= w;
		update_zero_flag(AC);
		update_negative_flag(AC);
	}

	void adc(Word w) {
		AC += w + (SR & Flag::Carry);
		update_zero_flag(AC);
		update_negative_flag(AC);
		// TODO update carry and overflow flags
	}

	void sbc(Word w) {
		AC -= w + (1 - (SR & Flag::Carry));
		update_zero_flag(AC);
		update_negative_flag(AC);
		// TODO update carry and overflow flags
	}

	void rol(Word& w) {
		Word new_carry = w >> 7;
		w = (w << 1) | (SR & Flag::Carry);
		update_zero_flag(w);
		update_negative_flag(w);
		assign_flags(Flag::Carry, new_carry);
	}

	void lda(Word w) { transfer(AC, w); }
	void ldx(Word w) { transfer(X, w); }
	void ldy(Word w) { transfer(Y, w); }

	void dex()        { decrement(X); }
	void dey()        { decrement(Y); }
	void inx()        { increment(X); }
	void iny()        { increment(Y); }
	void inc(Word& w) { increment(w); }

	void tax() { transfer(X, AC); }
	void tay() { transfer(Y, AC); }
	void txa() { transfer(AC, X); }
	void tya() { transfer(AC, Y); }
	void tsx() { transfer(X, SP); }
	void txs() { SP = X; }

	void clc() { clear_flags(Flag::Carry); }
	void cld() { clear_flags(Flag::Decimal); }
	void cli() { clear_flags(Flag::Interrupt); }
	void clv() { clear_flags(Flag::Overflow); }

	void sec() { set_flags(Flag::Carry); }
	void sed() { set_flags(Flag::Decimal); }
	void sei() { set_flags(Flag::Interrupt); }

};

struct Machine {
	static constexpr int memory_size = 1 << 16;

	std::array<Word, memory_size> memory;
	CPU cpu;

	Word read_word(uint16_t address) {
		return memory[address];
	}

	uint16_t read_long(uint16_t address) {
		return uint16_t(memory[address + 1]) << 8 | memory[address];
	}

	void normal_dispatch(Word opcode) {
		/*
		   ==== addressing modes ====
		   0 => indexed indirect (X)
		   1 => indirect indexed (Y)
		   2 => zeropage
		   3 => zeropage indexed (X)
		   4 => immediate
		   5 => absolute indexed (Y)
		   6 => absolute
		   7 => absolute indexed (X)

		   lo nibble gives the possible addressing modes
		   01    => 0 or 1
		   05 06 => 2 or 3
		   09    => 4 or 5
		   0d 0e => 6 or 7

		   parity of addressing mode is given by parity of hi nibble
		 */

		uint16_t address;
		uint8_t width;

		Word lo_op = opcode & 0x0f;
		Word hi_op = (opcode & 0xf0) >> 4;
		Word addressing_mode = ((lo_op - 1) >> 1) | (hi_op & 0x01);

		switch (addressing_mode) {
			case 0: // (indirect, x)
				address = read_long(read_word(cpu.PC + 1) + cpu.X);
				width = 2;
				break;
			case 1: // (indirect), y
				address = read_long(read_word(cpu.PC + 1)) + cpu.Y;
				width = 2;
				break;
			case 2: // zero page
				address = read_word(cpu.PC + 1);
				width = 2;
				break;
			case 3: // zero page+x
				address = Word(read_word(cpu.PC + 1) + cpu.X);
				width = 2;
				break;
			case 4: // immediate
				address = cpu.PC + 1;
				width = 2;
				break;
			case 5: // absolute, y
				address = read_long(cpu.PC + 1) + cpu.Y;
				width = 3;
				break;
			case 6: // absolute
				address = read_long(cpu.PC + 1);
				width = 3;
				break;
			case 7: // absolute, x
				address = read_long(cpu.PC + 1) + cpu.X;
				width = 3;
				break;
		}

		// TODO: choose the right instruction
		cpu.adc(memory[address]);
		cpu.PC += width;
	}

	void execute_instruction() {

		Word opcode = read_word(cpu.PC);
		Word lo_op = opcode & 0x0f;
		Word hi_op = opcode >> 4;

		switch (lo_op) {
			case 0x01:
			case 0x05: case 0x06:
			case 0x09:
			case 0x0D: case 0x0E:
			normal_dispatch(opcode);
		}

	}
};

int main () {
	CPU cpu;
}
