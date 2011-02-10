#!/usr/bin/env python

import optparse, sys, os, glob, re
import dbc.generator, dbc.db, dbc.parser, dbc.patch

parser = optparse.OptionParser( usage= "%prog [-otlbp] [ARGS]", version = "%prog 1.0" )
parser.add_option("-o", "--out", dest = "output_type", 
                  help    = "Output type (.cpp, .js) <NYI> [cpp]", metavar = "TYPE", 
                  default = "cpp", action = "store", type = "choice",
                  choices = [ 'cpp', 'js' ]), 
parser.add_option("-t", "--type", dest = "type", 
                  help    = "Processing type [spell]", metavar = "TYPE", 
                  default = "spell", action = "store", type = "choice",
                  choices = [ 'spell', 'class_list', 'talent', 'scale', 'view', 'header', 'patch', 'spec_spell_list', 'mastery_list', 'racial_list', 'glyph_list', 'class_flags', 'set_list', 'random_property_points', 'random_suffix', 'item_ench' ]), 
parser.add_option("-l", "--level", dest = "level", 
                  help    = "Scaling values up to level [85]", 
                  default = 85, action = "store", type = "int")
parser.add_option("-p", "--path", dest = "path", 
                  help    = "DBC input directory [cwd]", 
                  default = r'.', action = "store", type = "string")
parser.add_option("-b", "--build", dest = "build", 
                  help    = "World of Warcraft build number", 
                  default = 0, action = "store", type = "int")
parser.add_option("--prefix", dest = "prefix", 
                  help    = "Data structure prefix string", 
                  default = r'', action = "store", type = "string")
parser.add_option("--suffix", dest = "suffix", 
                  help    = "Data structure suffix string", 
                  default = r'', action = "store", type = "string")

(options, args) = parser.parse_args()

if options.build == 0 and options.type != 'header' and options.type != 'patch':
    parser.error('-b is a mandatory parameter for extraction type "%s"' % options.type)

if options.level % 5 != 0 or options.level > 100:
    parser.error('-l must be given as a multiple of 5 and be smaller than 100')

if options.type == 'view' and len(args) == 0:
    parser.error('View requires a DBC file name and an optional ID number')

if options.type == 'header' and len(args) == 0:
    parser.error('Header parsing requires at least a single DBC file to parse it from')

if options.type == 'patch' and len(args) < 3:
    parser.error('DBC patching requires at least 3 parameters, base_files output_dir patch_1_dir patch_2_dir ... patch_N_dir')
    
# Initialize the base model for dbc.data, creating the relevant classes for all patch levels
# up to options.build
dbc.data.initialize_data_model(options.build, dbc.data)

if options.type == 'spell':
    g = dbc.generator.SpellDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'class_list':
    g = dbc.generator.SpellListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'class_flags':
    g = dbc.generator.ClassFlagGenerator(options)
    if not g.initialize():
        sys.exit(1)
    #ids = g.filter(args[0])
    ids = g.filter()
    
    print g.generate(ids, args[0])
elif options.type == 'racial_list':
    g = dbc.generator.RacialSpellGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'mastery_list':
    g = dbc.generator.MasteryAbilityGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'spec_spell_list':
    g = dbc.generator.TalentSpecializationGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'random_property_points':
    g = dbc.generator.RandomPropertyPointsGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'random_suffix':
    g = dbc.generator.RandomSuffixGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'item_ench':
    g = dbc.generator.SpellItemEnchantmentGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'glyph_list':
    g = dbc.generator.GlyphListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'set_list':
    g = dbc.generator.ItemSetListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'header':
    dbcs = [ ]
    for fn in args:
        for i in glob.glob(fn):
            dbcs.append(i)
            
    for i in dbcs:
        dbc_file = dbc.parser.DBCParser(options, i)
        if not dbc_file.open_dbc():
            continue

        sys.stdout.write('%s: records=%d fields=%d record_size=%d string_block_size=%d\n' %
            ( i, dbc_file._records, dbc_file._fields, dbc_file._record_size, dbc_file._string_block_size ))
elif options.type == 'talent':
    g = dbc.generator.TalentDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    print g.generate(ids)
elif options.type == 'view':
    path = os.path.abspath(os.path.join(options.path, args[0]))
    id = None
    if len(args) > 1:
        id = int(args[1])

    dbc_file = dbc.parser.DBCParser(options, path)
    if not dbc_file.open_dbc():
        sys.exit(1)

    if id == None:
        record = dbc_file.next_record()
        while record != None:
            #mo = re.findall(r'(\$(?:{.+}|[0-9]*(?:<.+>|[^l][A-z:;]*)[0-9]?))', str(record))
            #if mo:
            #    print record, set(mo)
            #if (record.flags_1 & 0x00000040) == 0:
            sys.stdout.write('%s\n' % str(record))
            record = dbc_file.next_record()
    else:
        record = dbc_file.find(id)
        record.parse()
        print record
elif options.type == 'scale':
    g = dbc.generator.BaseScalingDataGenerator(options, [ 'gtChanceToMeleeCritBase', 'gtChanceToSpellCritBase' ] )
    if not g.initialize():
        sys.exit(1)
    print g.generate()

    g = dbc.generator.SpellScalingDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    print g.generate()

    g = dbc.generator.ClassScalingDataGenerator(options, [ 'gtChanceToMeleeCrit', 'gtChanceToSpellCrit', 'gtRegenMPPerSpt', 'gtOCTRegenMP' ] )
    if not g.initialize():
        sys.exit(1)
    print g.generate()

    g = dbc.generator.CombatRatingsDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    print g.generate()
elif options.type == 'patch':
    patch = dbc.patch.PatchBuildDBC(args[0], args[1], args[2:])
    patch.initialize()
    patch.PatchFiles()