// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/bit_field.h"
#include "common/common_types.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/common_funcs.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
namespace {
// https://forums.developer.nvidia.com/t/reverse-lut-for-lop3-lut/110651
// Emulate GPU's LOP3.LUT (three-input logic op with 8-bit truth table)
IR::U32 ApplyLUT(IR::IREmitter& ir, const IR::U32& a, const IR::U32& b, const IR::U32& c,
                 u64 ttbl) {
    switch (ttbl) {
        // generated code, do not edit manually
    case 0:
        return ir.Imm32(0);
    case 1:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseOr(b, c)));
    case 2:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 3:
        return ir.BitwiseNot(ir.BitwiseOr(a, b));
    case 4:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(c)))));
    case 5:
        return ir.BitwiseNot(ir.BitwiseOr(a, c));
    case 6:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 7:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseAnd(b, c)));
    case 8:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseAnd(b, c))));
    case 9:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseXor(b, c)));
    case 10:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(c)));
    case 11:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 12:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(b)));
    case 13:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(c)))));
    case 14:
        return ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseOr(b, c))));
    case 15:
        return ir.BitwiseNot(a);
    case 16:
        return ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseOr(b, c)));
    case 17:
        return ir.BitwiseNot(ir.BitwiseOr(b, c));
    case 18:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 19:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseAnd(a, c)));
    case 20:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseXor(a, b))));
    case 21:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseAnd(a, b)));
    case 22:
        return ir.BitwiseXor(a, ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(b, c)));
    case 23:
        return ir.BitwiseNot(
            ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c))));
    case 24:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c));
    case 25:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(b, c)));
    case 26:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 27:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 28:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseAnd(a, c)));
    case 29:
        return ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 30:
        return ir.BitwiseXor(a, ir.BitwiseOr(b, c));
    case 31:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseOr(b, c)));
    case 32:
        return ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 33:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseXor(a, c)));
    case 34:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 35:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 36:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 37:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(a, c)));
    case 38:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 39:
        return ir.BitwiseXor(ir.BitwiseOr(a, c), ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 40:
        return ir.BitwiseAnd(c, ir.BitwiseXor(a, b));
    case 41:
        return ir.BitwiseXor(a,
                             ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 42:
        return ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseAnd(a, b)));
    case 43:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)))));
    case 44:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b),
                             ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 45:
        return ir.BitwiseXor(a, ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 46:
        return ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(b, c));
    case 47:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 48:
        return ir.BitwiseAnd(a, ir.BitwiseNot(b));
    case 49:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(c)))));
    case 50:
        return ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseOr(a, c))));
    case 51:
        return ir.BitwiseNot(b);
    case 52:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseOr(a, ir.BitwiseNot(c)));
    case 53:
        return ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(a, ir.BitwiseNot(c)));
    case 54:
        return ir.BitwiseXor(b, ir.BitwiseOr(a, c));
    case 55:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseOr(a, c)));
    case 56:
        return ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseOr(a, c));
    case 57:
        return ir.BitwiseXor(b, ir.BitwiseOr(a, ir.BitwiseNot(c)));
    case 58:
        return ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(a, c));
    case 59:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 60:
        return ir.BitwiseXor(a, b);
    case 61:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseOr(a, c)));
    case 62:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 63:
        return ir.BitwiseNot(ir.BitwiseAnd(a, b));
    case 64:
        return ir.BitwiseAnd(a, ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 65:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseXor(a, b)));
    case 66:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 67:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseAnd(a, c)));
    case 68:
        return ir.BitwiseAnd(b, ir.BitwiseNot(c));
    case 69:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseAnd(a, ir.BitwiseNot(b))));
    case 70:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 71:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 72:
        return ir.BitwiseAnd(b, ir.BitwiseXor(a, c));
    case 73:
        return ir.BitwiseXor(a,
                             ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 74:
        return ir.BitwiseAnd(ir.BitwiseXor(a, c),
                             ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(b))));
    case 75:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 76:
        return ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseAnd(a, c)));
    case 77:
        return ir.BitwiseXor(a,
                             ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 78:
        return ir.BitwiseXor(ir.BitwiseAnd(a, c), ir.BitwiseOr(b, c));
    case 79:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(c)))));
    case 80:
        return ir.BitwiseAnd(a, ir.BitwiseNot(c));
    case 81:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(b)))));
    case 82:
        return ir.BitwiseAnd(ir.BitwiseXor(a, c), ir.BitwiseOr(a, ir.BitwiseNot(b)));
    case 83:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 84:
        return ir.BitwiseNot(ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseOr(a, b))));
    case 85:
        return ir.BitwiseNot(c);
    case 86:
        return ir.BitwiseXor(c, ir.BitwiseOr(a, b));
    case 87:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseOr(a, b)));
    case 88:
        return ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(a, c));
    case 89:
        return ir.BitwiseXor(c, ir.BitwiseOr(a, ir.BitwiseNot(b)));
    case 90:
        return ir.BitwiseXor(a, c);
    case 91:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 92:
        return ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(a, c));
    case 93:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseOr(a, ir.BitwiseNot(b))));
    case 94:
        return ir.BitwiseOr(ir.BitwiseXor(a, c), ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(b))));
    case 95:
        return ir.BitwiseNot(ir.BitwiseAnd(a, c));
    case 96:
        return ir.BitwiseAnd(a, ir.BitwiseXor(b, c));
    case 97:
        return ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, ir.BitwiseNot(b)),
                                              ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 98:
        return ir.BitwiseAnd(ir.BitwiseOr(a, c), ir.BitwiseXor(b, c));
    case 99:
        return ir.BitwiseNot(ir.BitwiseXor(b, ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 100:
        return ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(b, c));
    case 101:
        return ir.BitwiseNot(ir.BitwiseXor(c, ir.BitwiseAnd(a, ir.BitwiseNot(b))));
    case 102:
        return ir.BitwiseXor(b, c);
    case 103:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 104:
        return ir.BitwiseXor(a,
                             ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 105:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseXor(b, c)));
    case 106:
        return ir.BitwiseXor(c, ir.BitwiseAnd(a, b));
    case 107:
        return ir.BitwiseNot(
            ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(b, c))));
    case 108:
        return ir.BitwiseXor(b, ir.BitwiseAnd(a, c));
    case 109:
        return ir.BitwiseNot(
            ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, c), ir.BitwiseXor(b, c))));
    case 110:
        return ir.BitwiseNot(
            ir.BitwiseAnd(ir.BitwiseOr(a, ir.BitwiseNot(b)), ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 111:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseXor(b, c))));
    case 112:
        return ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseAnd(b, c)));
    case 113:
        return ir.BitwiseNot(
            ir.BitwiseXor(a, ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c))));
    case 114:
        return ir.BitwiseXor(ir.BitwiseOr(a, c), ir.BitwiseAnd(b, c));
    case 115:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(c)))));
    case 116:
        return ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(b, c));
    case 117:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(b)))));
    case 118:
        return ir.BitwiseOr(ir.BitwiseAnd(a, ir.BitwiseNot(b)), ir.BitwiseXor(b, c));
    case 119:
        return ir.BitwiseNot(ir.BitwiseAnd(b, c));
    case 120:
        return ir.BitwiseXor(a, ir.BitwiseAnd(b, c));
    case 121:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseOr(ir.BitwiseAnd(a, ir.BitwiseNot(b)), ir.BitwiseXor(b, c))));
    case 122:
        return ir.BitwiseOr(ir.BitwiseXor(a, c), ir.BitwiseAnd(a, ir.BitwiseNot(b)));
    case 123:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 124:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseAnd(a, ir.BitwiseNot(c)));
    case 125:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseXor(a, b))));
    case 126:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c));
    case 127:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseAnd(b, c)));
    case 128:
        return ir.BitwiseAnd(a, ir.BitwiseAnd(b, c));
    case 129:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c)));
    case 130:
        return ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseXor(a, b)));
    case 131:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 132:
        return ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 133:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, c), ir.BitwiseAnd(a, ir.BitwiseNot(b))));
    case 134:
        return ir.BitwiseXor(a,
                             ir.BitwiseOr(ir.BitwiseAnd(a, ir.BitwiseNot(b)), ir.BitwiseXor(b, c)));
    case 135:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseAnd(b, c)));
    case 136:
        return ir.BitwiseAnd(b, c);
    case 137:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseAnd(a, ir.BitwiseNot(b)), ir.BitwiseXor(b, c)));
    case 138:
        return ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(b))));
    case 139:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(b, c)));
    case 140:
        return ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(c))));
    case 141:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseOr(a, c), ir.BitwiseAnd(b, c)));
    case 142:
        return ir.BitwiseXor(a, ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c)));
    case 143:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseAnd(b, c))));
    case 144:
        return ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 145:
        return ir.BitwiseAnd(ir.BitwiseOr(a, ir.BitwiseNot(b)), ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 146:
        return ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, c), ir.BitwiseXor(b, c)));
    case 147:
        return ir.BitwiseNot(ir.BitwiseXor(b, ir.BitwiseAnd(a, c)));
    case 148:
        return ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(b, c)));
    case 149:
        return ir.BitwiseNot(ir.BitwiseXor(c, ir.BitwiseAnd(a, b)));
    case 150:
        return ir.BitwiseXor(a, ir.BitwiseXor(b, c));
    case 151:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c)))));
    case 152:
        return ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 153:
        return ir.BitwiseNot(ir.BitwiseXor(b, c));
    case 154:
        return ir.BitwiseXor(c, ir.BitwiseAnd(a, ir.BitwiseNot(b)));
    case 155:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(b, c)));
    case 156:
        return ir.BitwiseXor(b, ir.BitwiseAnd(a, ir.BitwiseNot(c)));
    case 157:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseOr(a, c), ir.BitwiseXor(b, c)));
    case 158:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseOr(a, ir.BitwiseNot(b)),
                                                            ir.BitwiseNot(ir.BitwiseXor(b, c)))));
    case 159:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseXor(b, c)));
    case 160:
        return ir.BitwiseAnd(a, c);
    case 161:
        return ir.BitwiseNot(
            ir.BitwiseOr(ir.BitwiseXor(a, c), ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(b)))));
    case 162:
        return ir.BitwiseAnd(c, ir.BitwiseOr(a, ir.BitwiseNot(b)));
    case 163:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(a, c)));
    case 164:
        return ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 165:
        return ir.BitwiseNot(ir.BitwiseXor(a, c));
    case 166:
        return ir.BitwiseNot(ir.BitwiseXor(c, ir.BitwiseOr(a, ir.BitwiseNot(b))));
    case 167:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseOr(a, b), ir.BitwiseXor(a, c)));
    case 168:
        return ir.BitwiseAnd(c, ir.BitwiseOr(a, b));
    case 169:
        return ir.BitwiseNot(ir.BitwiseXor(c, ir.BitwiseOr(a, b)));
    case 170:
        return c;
    case 171:
        return ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseOr(a, b)));
    case 172:
        return ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(a, ir.BitwiseNot(c)));
    case 173:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseXor(a, c), ir.BitwiseOr(a, ir.BitwiseNot(b))));
    case 174:
        return ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(b))));
    case 175:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(c)));
    case 176:
        return ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 177:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseAnd(a, c), ir.BitwiseOr(b, c)));
    case 178:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)))));
    case 179:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(ir.BitwiseAnd(a, c))));
    case 180:
        return ir.BitwiseXor(a, ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 181:
        return ir.BitwiseNot(
            ir.BitwiseAnd(ir.BitwiseXor(a, c), ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(b)))));
    case 182:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseNot(ir.BitwiseXor(b, c)))));
    case 183:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseXor(a, c)));
    case 184:
        return ir.BitwiseXor(ir.BitwiseOr(a, b), ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 185:
        return ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 186:
        return ir.BitwiseOr(c, ir.BitwiseAnd(a, ir.BitwiseNot(b)));
    case 187:
        return ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 188:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseAnd(a, c));
    case 189:
        return ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 190:
        return ir.BitwiseOr(c, ir.BitwiseXor(a, b));
    case 191:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 192:
        return ir.BitwiseAnd(a, b);
    case 193:
        return ir.BitwiseNot(
            ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(c)))));
    case 194:
        return ir.BitwiseNot(ir.BitwiseOr(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseOr(a, c))));
    case 195:
        return ir.BitwiseNot(ir.BitwiseXor(a, b));
    case 196:
        return ir.BitwiseAnd(b, ir.BitwiseOr(a, ir.BitwiseNot(c)));
    case 197:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(a, c)));
    case 198:
        return ir.BitwiseNot(ir.BitwiseXor(b, ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 199:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseOr(a, c)));
    case 200:
        return ir.BitwiseAnd(b, ir.BitwiseOr(a, c));
    case 201:
        return ir.BitwiseNot(ir.BitwiseXor(b, ir.BitwiseOr(a, c)));
    case 202:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 203:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 204:
        return b;
    case 205:
        return ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseOr(a, c)));
    case 206:
        return ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseOr(a, ir.BitwiseNot(c))));
    case 207:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(b)));
    case 208:
        return ir.BitwiseAnd(a, ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 209:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(b, c)));
    case 210:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 211:
        return ir.BitwiseNot(
            ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(c)))));
    case 212:
        return ir.BitwiseXor(
            a, ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 213:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseNot(ir.BitwiseAnd(a, b))));
    case 214:
        return ir.BitwiseNot(ir.BitwiseXor(
            a, ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c)))));
    case 215:
        return ir.BitwiseNot(ir.BitwiseAnd(c, ir.BitwiseXor(a, b)));
    case 216:
        return ir.BitwiseAnd(ir.BitwiseOr(a, c), ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 217:
        return ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 218:
        return ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(a, c));
    case 219:
        return ir.BitwiseNot(
            ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c))));
    case 220:
        return ir.BitwiseOr(b, ir.BitwiseAnd(a, ir.BitwiseNot(c)));
    case 221:
        return ir.BitwiseOr(b, ir.BitwiseNot(c));
    case 222:
        return ir.BitwiseOr(b, ir.BitwiseXor(a, c));
    case 223:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(c)))));
    case 224:
        return ir.BitwiseAnd(a, ir.BitwiseOr(b, c));
    case 225:
        return ir.BitwiseNot(ir.BitwiseXor(a, ir.BitwiseOr(b, c)));
    case 226:
        return ir.BitwiseNot(ir.BitwiseXor(ir.BitwiseAnd(a, b), ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 227:
        return ir.BitwiseNot(
            ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseNot(ir.BitwiseAnd(a, c))));
    case 228:
        return ir.BitwiseOr(ir.BitwiseAnd(a, c), ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 229:
        return ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 230:
        return ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(b, c));
    case 231:
        return ir.BitwiseNot(ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c)));
    case 232:
        return ir.BitwiseXor(a, ir.BitwiseAnd(ir.BitwiseXor(a, b), ir.BitwiseXor(a, c)));
    case 233:
        return ir.BitwiseNot(
            ir.BitwiseXor(a, ir.BitwiseOr(ir.BitwiseAnd(a, b), ir.BitwiseXor(b, c))));
    case 234:
        return ir.BitwiseOr(c, ir.BitwiseAnd(a, b));
    case 235:
        return ir.BitwiseOr(c, ir.BitwiseNot(ir.BitwiseXor(a, b)));
    case 236:
        return ir.BitwiseOr(b, ir.BitwiseAnd(a, c));
    case 237:
        return ir.BitwiseOr(b, ir.BitwiseNot(ir.BitwiseXor(a, c)));
    case 238:
        return ir.BitwiseOr(b, c);
    case 239:
        return ir.BitwiseNot(ir.BitwiseAnd(a, ir.BitwiseNot(ir.BitwiseOr(b, c))));
    case 240:
        return a;
    case 241:
        return ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseOr(b, c)));
    case 242:
        return ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseOr(b, ir.BitwiseNot(c))));
    case 243:
        return ir.BitwiseOr(a, ir.BitwiseNot(b));
    case 244:
        return ir.BitwiseOr(a, ir.BitwiseAnd(b, ir.BitwiseNot(c)));
    case 245:
        return ir.BitwiseOr(a, ir.BitwiseNot(c));
    case 246:
        return ir.BitwiseOr(a, ir.BitwiseXor(b, c));
    case 247:
        return ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseAnd(b, c)));
    case 248:
        return ir.BitwiseOr(a, ir.BitwiseAnd(b, c));
    case 249:
        return ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseXor(b, c)));
    case 250:
        return ir.BitwiseOr(a, c);
    case 251:
        return ir.BitwiseOr(a, ir.BitwiseNot(ir.BitwiseAnd(b, ir.BitwiseNot(c))));
    case 252:
        return ir.BitwiseOr(a, b);
    case 253:
        return ir.BitwiseOr(a, ir.BitwiseOr(b, ir.BitwiseNot(c)));
    case 254:
        return ir.BitwiseOr(a, ir.BitwiseOr(b, c));
    case 255:
        return ir.Imm32(0xFFFFFFFF);
        // end of generated code
    }
    throw NotImplementedException("LOP3 with out of range ttbl");
}

IR::U32 LOP3(TranslatorVisitor& v, u64 insn, const IR::U32& op_b, const IR::U32& op_c, u64 lut) {
    union {
        u64 insn;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_reg;
        BitField<47, 1, u64> cc;
    } const lop3{insn};

    if (lop3.cc != 0) {
        throw NotImplementedException("LOP3 CC");
    }

    const IR::U32 op_a{v.X(lop3.src_reg)};
    const IR::U32 result{ApplyLUT(v.ir, op_a, op_b, op_c, lut)};
    v.X(lop3.dest_reg, result);
    return result;
}

u64 GetLut48(u64 insn) {
    union {
        u64 raw;
        BitField<48, 8, u64> lut;
    } const lut{insn};
    return lut.lut;
}
} // Anonymous namespace

void TranslatorVisitor::LOP3_reg(u64 insn) {
    union {
        u64 insn;
        BitField<28, 8, u64> lut;
        BitField<38, 1, u64> x;
        BitField<36, 2, PredicateOp> pred_op;
        BitField<48, 3, IR::Pred> pred;
    } const lop3{insn};

    if (lop3.x != 0) {
        throw NotImplementedException("LOP3 X");
    }
    const IR::U32 result{LOP3(*this, insn, GetReg20(insn), GetReg39(insn), lop3.lut)};
    const IR::U1 pred_result{PredicateOperation(ir, result, lop3.pred_op)};
    ir.SetPred(lop3.pred, pred_result);
}

void TranslatorVisitor::LOP3_cbuf(u64 insn) {
    LOP3(*this, insn, GetCbuf(insn), GetReg39(insn), GetLut48(insn));
}

void TranslatorVisitor::LOP3_imm(u64 insn) {
    LOP3(*this, insn, GetImm20(insn), GetReg39(insn), GetLut48(insn));
}
} // namespace Shader::Maxwell
