#!/usr/bin/env python

import sys, struct

_REMOVE_FIELD = 0x00
_ADD_FIELD    = 0x01

_DIFF_DATA = {
    12694: { 
        'Spell.dbc' : [ 
            ( ( 'flags_12694', '%#.8x' ), _ADD_FIELD, 'flags_7' )
        ] 
    },
    12759: { 
        'SpellPower.dbc' : [ 
            ( 'unk_12759', _ADD_FIELD, 'id_display' ),
        ],
        'TalentTab.dbc' : [
            ( 'unk_12759_1', _ADD_FIELD, 'unk_8' ),
            ( 'unk_12759_2', _ADD_FIELD, 'unk_12759_1' ),
        ],
    },
}

# Base DBC fields, works for 12604, as that's our first DBC data version
_DBC_FIELDS = {
    'ChatChannels.dbc' : [
          'id', 'unk_1', 'unk_2', 'unk_3', 'unk_4'
    ],
    'ChrRaces.dbc' : [
          'id',       'unk_1',      'id_faction', 'unk_3',      'm_model',    'f_model',
          'unk_6',    'id_team',    'unk_8',      'unk_9',      'unk_10',     'unk_11',
          'id_movie', 'faction',    'ofs_name',   'ofs_f_name', 'ofs_n_name', 'unk_17',
          'unk_19',   'expansion',  'unk_21',     'unk_22',     'unk_23',     'unk_24'
    ],
    'GlyphProperties.dbc' : [
          'id',      ( 'id_spell', '%5u' ),  'flags',      'unk_3'
    ],
    'Item.dbc' : [
          'id',         'class',      'subclass', ( 'unk_3', '%d' ), 'material',
          'id_display', 'type_inv',   'sheath'
    ],
    'ItemSet.dbc' : [
          'id', 'ofs_name', 
          'id_item_1', 'id_item_2', 'id_item_3', 'id_item_4', 'id_item_5',
          'id_item_6', 'id_item_7', 'id_item_8', 'id_item_9', 'id_item_10',
          'id_item_11', 'id_item_12', 'id_item_13', 'id_item_14', 'id_item_15',
          'id_item_16', 'id_item_17',
          'id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4',
          'id_spell_5', 'id_spell_6', 'id_spell_7', 'id_spell_8',
          'n_items_1', 'n_items_2', 'n_items_3', 'n_items_4',
          'n_items_5', 'n_items_6', 'n_items_7', 'n_items_8',
          'id_req_skill', 'val_req_skill'
    ],
    'ItemReforge.dbc': [
         'id', 'src_stat', ( 'm_src', '%.2f' ), 'dst_stat', ( 'm_dst', '%.2f' )
    ],
    'ItemRandomProperties.dbc' : [
          'id',         'ofs_name_int',
          'id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4', 'id_spell_5', 
          'ofs_name_sfx'
    ],
    'ItemRandomSuffix.dbc' : [
          ( 'id','%4u' ),   'ofs_name_sfx',   'ofs_name_int',
          ( 'id_property_1', '%5u' ),  ( 'id_property_2',  '%5u' ), ( 'id_property_3', '%5u' ),  ( 'id_property_4', '%5u' ),  ( 'id_property_5', '%5u' ),
          ( 'property_pct_1', '%5u' ), ( 'property_pct_2', '%5u' ), ( 'property_pct_3', '%5u' ), ( 'property_pct_4', '%5u' ), ( 'property_pct_5', '%5u' )
    ],
    'RandPropPoints.dbc' : [
          ( 'id', '%3u' ),
          ( 'epic_points_1', '%4u' ), ( 'epic_points_2', '%4u' ), ( 'epic_points_3', '%4u' ), ( 'epic_points_4', '%4u' ), ( 'epic_points_5', '%4u' ),
          ( 'rare_points_1', '%4u' ), ( 'rare_points_2', '%4u' ), ( 'rare_points_3', '%4u' ), ( 'rare_points_4', '%4u' ), ( 'rare_points_5', '%4u' ),
          ( 'uncm_points_1', '%4u' ), ( 'uncm_points_2', '%4u' ), ( 'uncm_points_3', '%4u' ), ( 'uncm_points_4', '%4u' ), ( 'uncm_points_5', '%4u' ),
    ],
    'SkillLine.dbc' : [
          'id',         'id_category',    'id_skill_cost', 'ofs_name', 'ofs_desc',
          'spell_icon', 'alternate_verb', 'can_link'
    ],
    'SkillLineAbility.dbc' : [
          'id',          'id_skill',     'id_spell',        'mask_race',        'mask_class',
          'n_mask_race', 'n_mask_class', 'req_skill_value', 'id_spell_forward', 'learn_on_get_skill',
          'max_value',   'min_value',    'unk_12',          'unk_13'
    ],
    'SkillLineAbilitySortedSpell.dbc' : [
          'id',          'field'
    ],
    'SkillRaceClassInfo.dbc' : [
          'id',          'id_skill',      'mask_race',       'mask_class',       'flags',
          'req_level',   'id_skill_tier', 'id_skill_cost',   'unk'
    ],
    'Spell.dbc' : [
        ( 'id', '%5u' ),           ( 'flags', '%#.8x' ),      ( 'flags_1', '%#.8x' ),      ( 'flags_2', '%#.8x' ),      ( 'flags_3', '%#.8x' ),
        ( 'flags_4', '%#.8x' ),    ( 'flags_5', '%#.8x' ),    ( 'flags_6', '%#.8x' ),      ( 'flags_7', '%#.8x' ),      ( 'flags_8', '%#.8x' ), 
        ( 'id_cast_time', '%5u'),  ( 'id_duration', '%5u' ),  ( 'type_power', '%2d' ),     ( 'id_range', '%5u' ),       ( 'prj_speed', '%4.1f' ),
           'unk_15',                 'unk_16',                ( 'id_icon', '%5u' ),        ( 'id_active_icon', '%5u' ), ( 'ofs_name', '%5u' ),
        ( 'ofs_rank', '%5u' ),     ( 'ofs_desc', '%5u' ),     ( 'ofs_tool_tip', '%5u' ),   ( 'mask_school', '%#.2x' ),    'id_rune_cost',
        ( 'id_missile', '%5u' ),   ( 'id_desc_var', '%5u' ),  ( 'id_difficulty', '%5u' ),  ( 'extra_coeff', '%5.7f' ),                  ( 'id_scaling', '%5u' ), 
        ( 'id_aura_opt', '%5u' ),  ( 'id_aura_rest', '%5u' ), ( 'id_cast_req', '%5u' ),    ( 'id_categories', '%5u' ),  ( 'id_class_opts', '%5u' ),
        ( 'id_cooldowns', '%5u' ),   'unk_36',                ( 'id_equip_items', '%5u' ), ( 'id_interrupts', '%5u' ),  ( 'id_levels', '%5u' ),
        ( 'id_power', '%5u' ),     ( 'id_reagents', '%5u' ),  ( 'id_shapeshift', '%5u' ),  ( 'id_tgt_rest', '%5u' ),    ( 'id_totems', '%5u' ),
          'unk_45'
    ],
    'SpellAuraOptions.dbc' : [
          'id', ( 'stack_amount', '%3u' ), ( 'proc_chance', '%3u' ), ( 'proc_charges', '%2u' ), ( 'proc_flags', '%#.8x' )
    ],
    'SpellCastTimes.dbc' : [
          'id', ( 'cast_time', '%5d' ), ( 'cast_time_per_level', '%d' ), ( 'min_cast_time', '%5d' )
    ],
    'SpellClassOptions.dbc' : [
          'id', 'modal_next_spell', ( 'spell_family_flags_1', '%#.8x' ), ( 'spell_family_flags_2', '%#.8x' ), ( 'spell_family_flags_3', '%#.8x' ), 'spell_family_name', 'desc'
    ],
    'SpellCategory.dbc' : [
          'id', 'field'
    ],
    'SpellCategories.dbc' : [
          'id',            ( 'category', '%4u' ), 'dmg_class', 'dispel', 'mechanic', 
          'type_prevention', 'start_recovery_category'
    ],
    'SpellCooldowns.dbc' : [
        'id', ( 'category_cooldown', '%7u' ), ( 'cooldown', '%7u' ), ( 'gcd_cooldown', '%4u' )
    ],
    'SpellDescriptionVariables.dbc' : [
        'id', 'ofs_var'
    ],
    'SpellDuration.dbc'  : [
        'id', ( 'duration_1', '%9d' ), 'duration_2', 'duration_3'
    ],
    'SpellEffect.dbc' : [
        ( 'id', '%5u' ),         ( 'type', '%4u' ),            ( 'multiple_value', '%f' ), ( 'sub_type', '%4u' ),     ( 'amplitude', '%5u' ),
        ( 'base_value', '%7d' ), ( 'coefficient', '%13.10f' ), ( 'dmg_multiplier', '%f' ),   'chain_target',          ( 'die_sides', '%2d' ),
          'item_type',             'mechanic',                 ( 'misc_value', '%7d' ),    ( 'misc_value_2', '%7d' ), ( 'points_per_combo_points', '%5.1f' ), 
          'id_radius',             'id_radius_max',            ( 'real_ppl', '%5.3f' ),       ( 'class_mask_1', '%#.8x' ), ( 'class_mask_2', '%#.8x' ),
          ( 'class_mask_3', '%#.8x' ),       ( 'trigger_spell', '%5d' ),     'implicit_target_1',        'implicit_target_2',     ( 'id_spell', '%5u' ),
        ( 'index', '%u' )
    ],
    'SpellItemEnchantment.dbc' : [
          ( 'id', '%4u' ), 'charges', 
          ( 'type_1', '%3u' ), ( 'type_2', '%3u' ), ( 'type_3', '%3u' ), 
          'amount_1', 'amount_2', 'amount_3', 'amount_4', 'amount_5', 'amount_6', 
          ( 'id_property_1', '%5u' ), ( 'id_property_2', '%5u' ), ( 'id_property_3', '%5u' ), 
          'ofs_desc', 'id_aura', 'slot', 'id_gem', 'enchantment_condition', 
          'req_skill', 'req_skill_value', 'unk_1', 'unk_2'
    ],
    'SpellLevels.dbc' : [
          'id', ( 'base_level', '%3u' ), ( 'max_level', '%2u' ), 'spell_level'
    ],
    'SpellPower.dbc' : [
          'id', ( 'cost', '%6u' ), 'cost_pl', ( 'cost_perc', '%3u' ), 'per_second',
          'id_display'
    ],
    'SpellRadius.dbc' : [
          'id', ( 'radius_1', '%7.1f' ), 'radius_2', ( 'radius_3', '%7.f' )
    ],
    'SpellRange.dbc' : [
          'id',       ( 'min_range', '%7.1f' ), ( 'min_range_2', '%5.1f' ), ( 'max_range', '%7.1f' ), ( 'max_range_2', '%5.1f' ),
          'id_range',   'unk_6',                  'unk_7'
    ],
    'SpellRuneCost.dbc' : [
          'id', 'rune_cost_1', 'rune_cost_2', 'rune_cost_3', ( 'rune_power_gain', '%3u' )
    ],
    'SpellScaling.dbc' : [
          'id',                      ( 'cast_min', '%5d' ),       ( 'cast_max', '%5d' ),       ( 'cast_div', '%2u' ),     ( 'id_class', '%2d' ),
        ( 'e1_average', '%13.10f' ), ( 'e2_average', '%13.10f' ), ( 'e3_average', '%13.10f' ), ( 'e1_delta', '%13.10f' ), ( 'e2_delta', '%13.10f' ),
        ( 'e3_delta', '%13.10f' ),   ( 'e1_bcp', '%13.10f' ),     ( 'e2_bcp', '%13.10f' ),     ( 'e3_bcp', '%13.10f' ),    ( 'c_scaling', '%13.10f' ),
          ( 'c_scaling_threshold', '%2u' )
    ],
    'Talent.dbc' : [
        ( 'id', '%5u' ),          'talent_tab',         ( 'row', '%2u' ),         ( 'col', '%2u' ),       ( 'id_rank_1', '%5u' ),
        ( 'id_rank_2', '%5u' ), ( 'id_rank_3', '%5u' ), ( 'id_rank_4', '%5u' ),   ( 'id_rank_5', '%5u' ), ( 'talent_depend', '%5u' ),
          'unk_10',               'unk_11',             ( 'depend_rank', '%2u' ),   'unk_13',               'unk_14', 
          'unk_15',               'unk_16',               'unk_17',                 'unk_18'
    ],
    'TalentTab.dbc' : [
        ( 'id', '%5u' ),       'ofs_name',          'spell_icon', ( 'mask_class', '%#.3x' ), ( 'mask_pet_talent', '%#.1x' ),
        ( 'tab_page', '%2u' ), 'ofs_internal_name', 'ofs_desc',     'unk_8'
    ],
    'TalentTreePrimarySpells.dbc' : [
          'id', 'id_talent_tab', 'id_spell', 'field_3'
    ],
    'gtChanceToMeleeCrit.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToMeleeCritBase.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToSpellCrit.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToSpellCritBase.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtCombatRatings.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTClassCombatRatingScalar.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTRegenMP.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtRegenMPPerSpt.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtSpellScaling.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
}

class DBCRecord(object):
    __default = None

    @classmethod
    def default(cls, *args):
        if not cls.__default:
            cls.__default = cls(None, None)

        # Ugly++ but it will have to do
        for i in xrange(0, len(args), 2):
            setattr(cls.__default, args[i], args[i + 1])

        return cls.__default
        
    @classmethod
    def data_size(cls):
        return struct.Struct(''.join(cls._format)).size

    def __init__(self, parser, record):
        self._record     = record
        self._dbc_parser = parser
        
    def parse(self):
        if not self._record: return None
        record_tuple = struct.unpack(''.join(self._format), self._record)
        
        for i in xrange(0, len(record_tuple)):
            setattr(self, self._fields[i], record_tuple[i])
        
        return self

    def value(self, *args):
        v = [ ]
        for attr in args:
            if attr in dir():
                v += [ getattr(self, attr) ]
            else:
                v += [ None ]

        return v

    def field(self, *args):
        f = [ ]
        for field in args:
            if field in self._fields:
                f += [ self._field_fmt[self._fields.index(field)] % getattr(self, field) ]
            else:
                f += [ None ]

        return f
    
    def __str__(self):
        s = ''

        for i in self._fields:
            s += '%s=%s ' % (i, (self._field_fmt[self._fields.index(i)] % getattr(self, i)).strip())

        return s

class Spell(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self._effects = [ None, None, None ]
        self.name     = ''

    def add_effect(self, spell_effect):
        self._effects[spell_effect.index] = spell_effect
        setattr(self, 'effect_%d' % ( spell_effect.index + 1 ), spell_effect)
    
    def __getattr__(self, name):
        # Hack to get effect default values if spell has no effect_x, as those fields 
        # are the only ones that may be missing in Spellxxx. Just in case raise an 
        # attributeerror if something else is being accessed
        if 'effect_' in name:
            return SpellEffect.default()
        
        raise AttributeError

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'name' in args:
            f[args.index('name')] = '%-36s' % ('"%s"' % self.name)
           
        for field_id in [ 'desc', 'tt', 'rank' ]:
            if field_id in args:
                v = getattr(self, field_id)
                if v == 0:
                    f[args.index(field_id)] = repr('%s' % v)[1:-1]
                else:
                    s = repr(v)
                    s = s[1:-1]
                    s = s.replace(r'"', r'\"')
                    f[args.index(field_id)] = '"%s"' % s

        return f

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = 0

        if self.ofs_rank != 0:
            self.rank = self._dbc_parser.get_string_block(self.ofs_rank)
        else:
            self.rank = 0

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = 0

        if self.ofs_tool_tip != 0:
            self.tt = self._dbc_parser.get_string_block(self.ofs_tool_tip)
        else:
            self.tt = 0

        return self

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += ' name=\"%s\" ' % self.name
        if self.rank != 0:
            s += 'rank=\"%s\" ' % self.rank

        if self.desc != 0:
            s += 'desc=\"%s\" ' % self.desc

        if self.tt != 0:
            s += 'tt=\"%s\" ' % self.tt

        for i in xrange(len(self._effects)):
            if not self._effects[i]:
                continue
            s += 'effect_%d={ %s} ' % (i + 1, str(self._effects[i]))

        return s

class SpellCooldowns(DBCRecord):
    def __getattribute__(self, name):
        if name == 'cooldown_duration':
            if self.category_cooldown > 0:
                return self.category_cooldown
            elif self.cooldown > 0:
                return self.cooldown
            else:
                return 0
        else:
            return DBCRecord.__getattribute__(self, name)

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'cooldown_duration' in args:
            if self.cooldown > 0:
                f[args.index('cooldown_duration')] = '%7u' % self.cooldown
            elif self.category_cooldown > 0:
                f[args.index('cooldown_duration')] = '%7u' % self.category_cooldown
            else:
                f[args.index('cooldown_duration')] = '%7u' % 0

        return f

class SpellPower(DBCRecord):
    def __getattribute__(self, name):
        if name == 'power_cost':
            if self.cost > 0:
                return self.cost
            elif self.cost_perc > 0:
                return self.cost_perc
            else:
                return 0
        else:
            return DBCRecord.__getattribute__(self, name)
    
    def field(self, *args):
        f = DBCRecord.field(self, *args)

        # Add a special 'power_cost' field kludge
        if 'power_cost' in args:
            if self.cost > 0:
                f[args.index('power_cost')] = '%3u' % self.cost
            elif self.cost_perc > 0:
                f[args.index('power_cost')] = '%3u' % self.cost_perc
            else:
                f[args.index('power_cost')] = '%3u' % 0

        return f

class TalentTab(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_internal_name != 0:
            self.internal_name = self._dbc_parser.get_string_block(self.ofs_internal_name)
        else:
            self.internal_name = ''

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = 'name="%s" internal_name="%s" ' % (self.name, self.internal_name)

        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        s += DBCRecord.__str__(self)

        return s

class ItemRandomSuffix(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name_sfx != 0:
            self.name_sfx = self._dbc_parser.get_string_block(self.ofs_name_sfx)
        else:
            self.name_sfx = ''

        if self.ofs_name_int != 0:
            self.name_int = self._dbc_parser.get_string_block(self.ofs_name_int)
        else:
            self.name_int = ''

    def __str__(self):
        s = ''

        if self.name_sfx:
            s += 'suffix=\"%s\" ' % self.name_sfx

        if self.name_int:
            s += 'internal_name=\"%s\" ' % self.name_int

        s += DBCRecord.__str__(self)

        return s

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'suffix' in args:
            f[args.index('suffix')] = '%-22s' % ('"%s"' % self.name_sfx)

        return f

class ItemRandomProperties(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name_int != 0:
            self.name_int = self._dbc_parser.get_string_block(self.ofs_name_int)
        else:
            self.name_int = ''

        if self.ofs_name_sfx != 0:
            self.name_sfx = self._dbc_parser.get_string_block(self.ofs_name_sfx)
        else:
            self.name_sfx = ''

    def __str__(self):
        s = ''

        if self.name_int:
            s += 'name_int=\"%s\" ' % self.name_int

        if self.name_sfx:
            s += 'name_sfx=\"%s\" ' % self.name_sfx

        s += DBCRecord.__str__(self)

        return s

class SkillLine(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = 'name="%s" ' % self.name

        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        s += DBCRecord.__str__(self)

        return s

class SpellItemEnchantment(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = ''
        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        s += DBCRecord.__str__(self)

        return s

class SpellDescriptionVariables(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_var != 0:
            self.var = self._dbc_parser.get_string_block(self.ofs_var)
        else:
            self.var = ''

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'var' in args:
            #print dir(self), self.ofs_var
            v = getattr(self, 'var')
            if v == 0:
                f[args.index('var')] = repr('%s' % v)[1:-1]
            else:
                s = repr(v)
                s = s[1:-1]
                s = s.replace(r'"', r'\"')
                f[args.index('var')] = '"%s"' % s

        return f
    
    def __str__(self):
        s = DBCRecord.__str__(self)

        if self.var:
            s += 'var=\"%s\" ' % self.var

        return s

class ItemSet(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''
    
    def __str__(self):
        s = ''
        if self.name:
            s += 'name=\"%s\" ' % self.name

        s += DBCRecord.__str__(self)

        return s

class ChrRaces(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_f_name != 0:
            self.f_name = self._dbc_parser.get_string_block(self.ofs_f_name)
        else:
            self.f_name = ''
    
        if self.ofs_n_name != 0:
            self.n_name = self._dbc_parser.get_string_block(self.ofs_n_name)
        else:
            self.n_name = ''

    def __str__(self):
        if self.name:
            s = 'name="%s" ' % self.name

        if self.f_name:
            s += 'female_name=\"%s\" ' % self.f_name
            
        if self.n_name:
            s += 'neutral_name=\"%s\" ' % self.n_name

        s += DBCRecord.__str__(self)

        return s

class SkillLineCategory(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''
    
    def __str__(self):
        s = 'name="%s" ' % self.name

        s += DBCRecord.__str__(self)

        return s
        
class GameTables(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_data_file != 0:
            self.data_file = self._dbc_parser.get_string_block(self.ofs_data_file)
        else:
            self.data_file = ''
    
    def __str__(self):
        s = 'data_file="%s" ' % self.data_file

        s += DBCRecord.__str__(self)

        return s
        
def initialize_data_model(build, obj):
    # First, create base classes, based on build id 0
    for dbc_file_name, field_data in _DBC_FIELDS.iteritems():
        class_name = '%s' % dbc_file_name.split('.')[0]
        cls_fields = [ ]
        cls_format = [ ]
        cls_field_fmt = [ ]
    
        if class_name not in dir(obj):
            setattr(obj, class_name, type(r'%s' % class_name, ( DBCRecord, ), dict( DBCRecord.__dict__ )))
        
        cls = getattr(obj, class_name)
        
        for field in field_data:
            if isinstance(field, tuple):
                cls_fields.append(field[0])
                cls_field_fmt.append(field[1])
                if 'x' in field[1] or 'u' in field[1]:
                    cls_format.append('I')
                elif 'f' in field[1]:
                    cls_format.append('f')
                elif 'd' in field[1]:
                    cls_format.append('i')
                else:
                    sys.stderr.write('Unknown formatting attribute %s for %s\n' % ( field[1], field[0] ))
                
                setattr(cls, field[0], 0)
            else:
                cls_fields.append(field)
                cls_field_fmt.append('%u')
                cls_format.append('I')
                setattr(cls, field, 0)
                
        setattr(cls, '_fields', cls_fields)
        setattr(cls, '_format', cls_format)
        setattr(cls, '_field_fmt', cls_field_fmt)

    # Then, derive patch classes from base
    for build_id in sorted(_DIFF_DATA.keys()):
        for dbc_file_name, dbc_fields in _DBC_FIELDS.iteritems():
            class_base_name = dbc_file_name.split('.')[0]
            class_name      = r'%s%d' % ( class_base_name, build )
            dbc_diff_data   = _DIFF_DATA.get(build_id, { }).get(dbc_file_name)

            if class_name not in dir(obj):
                parent_class = getattr(obj, '%s' % class_base_name)
                setattr(obj, 
                    r'%s%d' % ( class_base_name, build ), 
                    type(r'%s%d' % ( class_base_name, build ), ( parent_class, ), dict(parent_class.__dict__))
                )

            cls             = getattr(obj, '%s%d' % ( class_base_name, build ) )

            if not dbc_diff_data:
                continue
            
            # If there's some diff data for us, we need to go through it and modify the created class
            # Accordingly
            for diff_data in dbc_diff_data:
                if diff_data[1] == _ADD_FIELD:
                    idx_field = cls._fields.index(diff_data[2])
                    if idx_field < 0:
                        sys.stderr.write('Unable to find field %s for build %d for addition\n' % ( diff_data[2], build_id ))
                        continue

                    field_name = diff_data[0]
                    field_format = '%u'
                    if isinstance(diff_data[0], tuple):
                        field_name = diff_data[0][0]
                        field_format = diff_data[0][1]
                        
                    cls._fields.insert(idx_field + 1, field_name)
                    if 'x' in field_format or 'u' in field_format:
                        cls._format.insert(idx_field + 1, 'I')
                    elif 'f' in field_format:
                        cls._format.insert(idx_field + 1, 'f')
                    elif 'd' in field_format:
                        cls._format.insert(idx_field + 1, 'i')
                    else:
                        sys.stderr.write('Unknown formatting attribute %s for %s\n' % ( field_format, field_name ))
                    
                    cls._field_fmt.insert(idx_field + 1, field_format)
                    setattr(cls, field_name, 0)
                elif diff_data[1] == _REMOVE_FIELD:
                    idx_field = cls._fields.index(diff_data[0])
                    if idx_field < 0:
                        sys.stderr.write('Unable to find field %s for build %d for removal\n' % ( diff_data[0], build_id ))
                        continue
                    
                    del cls._format[idx_field]
                    del cls._fields[idx_field]
                    del cls._field_fmt[idx_field]
                    delattr(cls, diff_data[0])

        if build_id >= build:
            break
