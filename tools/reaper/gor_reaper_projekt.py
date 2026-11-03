#!/usr/bin/env python3
# Bygger testlåtarna som Reaper-projekt (.RPP), ett spår med Kroppsmaskinen i varje:
#
#   Fabriksgolv  – grind 2-låten: mönsterbyten, tyst/på och Spänningen som automation, och en MIDI-region
#                  som transponerar basgångarna.
#   Stalverket, Antwerpen – demolåtarna (fas 4): presetet har en låt, och låtläget sköter hela
#                  arrangemanget; projektet är bara spåret, tempot och längden.
#
#   python3 tools/reaper/gor_reaper_projekt.py <KMRender> <utkatalog> [--render] [--reaper-id <tal>]
#
# Skriver för varje låt:
#   <utkatalog>/<Namn>.RPP   – öppnas i Reaper
#   <utkatalog>/<Namn>.json  – samma låt för "KMRender --projekt" (spelas utan DAW)
#   <utkatalog>/<Namn>.wav   – (med --render) förhandslyssning, renderad av KMRender
#
# Pluginets läge (preset + extra mönster) hämtas med "KMRender --state", och parametrarnas ordning med
# "KMRender --parametrar", så att automationen hamnar på rätt parameter.
#
# Reaper känner igen VST3-plugin på ett tal och ett GUID. GUID:et följer av pluginkoderna (Kpmk, Km87);
# talet läses ur Reapers pluginlista (~/Library/Application Support/REAPER/reaper-vstplugins*.ini) när
# skriptet körs på Macen där Kroppsmaskinen är inläst i Reaper. Annars används --reaper-id eller ett
# reservvärde.
#
# Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
import base64, glob, json, os, re, struct, subprocess, sys, tempfile, uuid

PPQ = 960                                  # Reapers MIDI-upplösning: tick per fjärdedel
GUID = 'ABCDEF019182FAEB4B706D6B4B6D3837'  # JUCE: ABCDEF01 9182FAEB + 'Kpmk' + 'Km87'
RESERV_ID = 1265401451                     # används om Reapers pluginlista inte går att läsa ('Kpmk')

# --- Låten ----------------------------------------------------------------------------------------

def takt(t):
    """Takt t (från 1) i fjärdedelar."""
    return (t - 1) * 4.0

def fabriksgolv():
    """Grind 2: Fabriksgolv, 125 BPM, 112 takter (3:35). Bygger på presetet Fabriksgolv 125 med fler mönster."""
    monster_texter = {
        # Stampverket: 2 = full, 3 = break (hi-hat och metall), 4 = intro (bastrumma och hi-hat)
        'monster:sv:2A': '2000100010001000/0000100000001000/0000000000000000/0000100000001000/1121112111211121/0000000000100000',
        'monster:sv:2B': '2000100010001010/000010000000102{r1}2/0000000000001000/0000100000001000/1121112111211121/0000000000100010',
        'monster:sv:3A': '0000000000000000/0000000000000000/0000000000000000/0000000000000000/1010101010101010/0000000000000000'
                         '/0000000000000000/0000000000000000/0000000000000000/1000000010000000',
        'monster:sv:4A': '2000100010001000/0000000000000000/0000000000000000/0000000000000000/0010001000100010',
    }
    # Stampverket växlar A och B varannan takt (A+B i presetet): breaket och introt är likadana i båda.
    monster_texter['monster:sv:3B'] = monster_texter['monster:sv:3A']
    monster_texter['monster:sv:4B'] = monster_texter['monster:sv:4A']
    monster_texter.update({
        # Kolvbasen: 2 = mer rörelse, 3 = halvtid i breaket
        'monster:kb:2A': '0a 0 0u 0 0a 0 0u 0 3a 3 3u 3 -2a -2 -2u 0~',
        'monster:kb:3A': '0a/5 . . . . . . . -2a/5 . . . . . . .',
        # Hugget: 2 = fler stötar
        'monster:hu:2A': '0a/1 . . 0/0 . . 0a/1 . . . 3a/1 . . . -2a/1 .',
    })
    K = {'kb': 'km_kb_', 'kb2': 'km_kb2_', 'sv': 'km_sv_', 'hu': 'km_hu_'}
    auto = {}
    def satt(param, t, varde, fore=False):
        """Sätter param från takt t. Mönsterbyten läggs en sextondel före taktstrecket (de tar över vid takten)."""
        auto.setdefault(param, []).append([takt(t) - (0.25 if fore and t > 1 else 0.0), varde])
    def monster(kanal, t, nr):
        satt(K[kanal] + 'monster', t, nr, fore=True)
    def tyst(kanal, t, pa):
        satt(K[kanal] + 'tyst', t, 1 if pa else 0)

    # A: intro – bara trummor
    monster('sv', 1, 4); monster('kb', 1, 1); monster('hu', 1, 1)
    tyst('kb', 1, True); tyst('kb2', 1, True); tyst('hu', 1, True); tyst('sv', 1, False)
    satt('km_spanning', 1, 0.0)
    # B: Kolvbasen kommer in
    tyst('kb', 9, False); monster('sv', 9, 1)
    # C: andra basen och stötarna
    tyst('kb2', 17, False); tyst('hu', 17, False)
    # D: mer rörelse
    monster('kb', 33, 2); monster('hu', 33, 2); satt('km_spanning', 33, 0.3)
    # E: break – halvtidsbas och hi-hat, Spänningen byggs upp
    monster('sv', 41, 3); monster('kb', 41, 3); tyst('kb2', 41, True); tyst('hu', 41, True)
    for i, s in enumerate([0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]):
        satt('km_spanning', 41 + i, s)
    # F: fullt
    monster('sv', 49, 2); monster('kb', 49, 2); monster('hu', 49, 2)
    tyst('kb2', 49, False); tyst('hu', 49, False); satt('km_spanning', 49, 0.5)
    # G: avbrott – grundmönstren, utan stötar
    monster('sv', 73, 1); monster('kb', 73, 1); tyst('hu', 73, True); satt('km_spanning', 73, 0.2)
    # H: fullt igen, mest spänning
    monster('sv', 81, 2); monster('kb', 81, 2); tyst('hu', 81, False); satt('km_spanning', 81, 0.7)
    # I: utklingning – kanalerna försvinner en i taget
    tyst('hu', 97, True); satt('km_spanning', 97, 0.3)
    tyst('kb2', 101, True)
    tyst('kb', 105, True); monster('sv', 105, 4)
    tyst('sv', 112, True)

    # Transponering: tangenterna flyttar basgångarna (grundtonen A1 = 33). Takt 57–64 en kvart upp (D2).
    noter = [[takt(57) - 0.25, 0.2, 38, 0.8], [takt(65) - 0.25, 0.2, 33, 0.8]]

    automation = [{'param': p, 'punkter': sorted(pts)} for p, pts in sorted(auto.items())]
    return {'titel': 'Fabriksgolv – grind 2', 'preset': 'Fabriksgolv 125', 'tempo': 125, 'langdSlag': takt(113),
            'monster': monster_args(monster_texter), 'automation': automation, 'noter': noter}

def demo(titel, preset, filnamn):
    """En demolåt: presetets låt spelas i låtläget. Tempot och längden läses ur läget (se main)."""
    return {'titel': titel, 'preset': preset, 'fil': filnamn, 'monster': [], 'automation': [], 'noter': [],
            'tempo': None, 'langdSlag': None,
            'anteckning': 'Arrangemanget ligger i pluginets låt (låtläget, vyn LÅTEN); spåret har ingen automation.'}

def las_lat(state_xml):
    """Tempot och låtens längd i fjärdedelar (plus en takt för utklingningen) ur pluginets läge."""
    tempo = float(re.search(r'id="km_tempo" value="([^"]*)"', state_xml).group(1))
    text = re.search(r'<Laten text="([^"]*)"', state_xml).group(1)
    takter = sum(int(r.split()[0]) for r in text.split(';') if r.strip())
    return tempo, (takter + 1) * 4.0

def monster_args(m):
    return ['%s=%s' % (k, v) for k, v in m.items()]

# --- Reaper-formatet --------------------------------------------------------------------------------

def reaper_id():
    """Talet som Reaper använder för Kroppsmaskinen, ur Reapers pluginlista om den finns."""
    if '--reaper-id' in sys.argv:
        return int(sys.argv[sys.argv.index('--reaper-id') + 1])
    for ini in glob.glob(os.path.expanduser('~/Library/Application Support/REAPER/reaper-vst*plugins*.ini')):
        for rad in open(ini, encoding='utf-8', errors='replace'):
            if rad.lower().startswith('kroppsmaskinen.vst3='):
                falt = rad.split('=', 1)[1].split(',')
                if len(falt) > 1 and '{' in falt[1]:
                    return int(falt[1].split('{')[0])
    print('Hittade inte Kroppsmaskinen i Reapers pluginlista; använder reservvärdet %d.' % RESERV_ID)
    return RESERV_ID

def vst3_block(state_xml, program, rid):
    """Reapers VST3-block: [huvud][komponenttillstånd + kontrollertillstånd][programnamn], där
    komponenttillståndet är JUCE:s VST3-tillstånd: 'VC2!' + längd + XML + JUCE:s privata data."""
    xml = state_xml.encode('utf-8')
    privat = bytes.fromhex('0000000000000000004a554345507269766174654461746100010142797061737300010103001d00000000000000'
                           '4a5543455072697661746544617461')
    komponent = b'VC2!' + struct.pack('<I', len(xml)) + xml + privat
    kropp = struct.pack('<II', len(komponent), 1) + komponent + bytes(8)
    huvud = struct.pack('<11I', rid, 4276969198, 0, 2, 1, 0, 2, 0, len(kropp), 1, 0)
    namn = b'\x00' + program.encode('utf-8') + bytes(5)
    rader = [base64.b64encode(huvud).decode()]
    for i in range(0, len(kropp), 96):
        rader.append(base64.b64encode(kropp[i:i + 96]).decode())
    rader.append(base64.b64encode(namn).decode())
    return rader

def midi_handelser(noter, langd):
    h = []
    for start, l, ton, vel in noter:
        pa = int(round(start * PPQ)); av = int(round((start + l) * PPQ)) - 1
        h.append((pa, 1, 0x90, ton, max(1, min(127, int(round(vel * 127))))))
        h.append((max(pa + 1, av), 0, 0x80, ton, 0))
    h.sort()
    ut, senast = [], 0
    for t, _, status, d1, d2 in h:
        ut.append('E %d %02x %02x %02x' % (t - senast, status, d1, d2))
        senast = t
    ut.append('E %d b0 7b 00' % max(0, int(langd * PPQ) - senast))
    return ut

def guid():
    return '{' + str(uuid.uuid4()).upper() + '}'

def gor_rpp(lat, state_xml, index, intervall, rid):
    tempo, slag = lat['tempo'], lat['langdSlag']
    sek = lambda s: s * 60.0 / tempo
    ut = ['<REAPER_PROJECT 0.1 "7.0/macOS-arm64" 0 0',
          '  <NOTES 0 2', '    |%s – byggd av tools/reaper/gor_reaper_projekt.py' % lat['titel'],
          '    |' + lat.get('anteckning', 'Mönster, tyst och Spänningen är automation på Kroppsmaskinen; MIDI-regionen transponerar basgångarna.'), '  >',
          '  TEMPO %g 4 4 0' % tempo, '  SAMPLERATE 48000 0 0', '  LOOP 0',
          '  MASTER_VOLUME 1 0 -1 -1 1', '  SELECTION 0 %g' % sek(slag), '  CURSOR 0',
          '  <TRACK %s' % guid(), '    NAME "Kroppsmaskinen"', '    VOLPAN 1 0 -1 -1 1', '    MUTESOLO 0 0 0',
          '    NCHAN 2', '    FX 1', '    MAINSEND 1 0',
          '    <FXCHAIN', '      SHOW 0', '      LASTSEL 0', '      DOCKED 0', '      BYPASS 0 0 0',
          '      <VST "VST3i: Kroppsmaskinen (Kroppsmaskinen)" Kroppsmaskinen.vst3 0 "" %d{%s} ""' % (rid, GUID)]
    ut += ['        ' + r for r in vst3_block(state_xml, lat['preset'], rid)]
    ut += ['      >', '      FLOATPOS 0 0 0 0', '      FXID %s' % guid()]
    for k in lat['automation']:
        i = index[k['param']]
        lo, hi = intervall[k['param']]
        norm = lambda v: (v - lo) / (hi - lo) if hi > lo else 0.0
        ut += ['      <PARMENV %d 0.000000 1.000000 %.6f' % (i, norm(k['punkter'][0][1])),
               '        EGUID %s' % guid(), '        ACT 1 -1', '        VIS 1 1 1', '        LANEHEIGHT 0 0',
               '        ARM 0', '        DEFSHAPE 1 -1 -1']
        ut += ['        PT %.6f %.6f 1' % (sek(max(0.0, s)), norm(v)) for s, v in k['punkter']]
        ut += ['      >']
    ut += ['      WAK 0 0', '    >']
    if lat['noter']:
        ut += ['    <ITEM', '      POSITION 0', '      LENGTH %.6f' % sek(slag), '      LOOP 0', '      NAME "Transponering"',
               '      IGUID %s' % guid(), '      <SOURCE MIDI', '        HASDATA 1 %d QN' % PPQ]
        ut += ['        ' + e for e in midi_handelser(lat['noter'], slag)]
        ut += ['        GUID %s' % guid(), '      >', '    >']
    ut += ['  >', '>']
    return '\n'.join(ut) + '\n'

def main():
    if len(sys.argv) < 3:
        print('gor_reaper_projekt.py <KMRender> <utkatalog> [--render] [--reaper-id <tal>]')
        sys.exit(1)
    kmrender, utkatalog = os.path.abspath(sys.argv[1]), sys.argv[2]
    os.makedirs(utkatalog, exist_ok=True)
    rid = reaper_id()
    latar = [dict(fabriksgolv(), fil='Fabriksgolv'),
             demo('Stålverket (demo)', 'Stålverket 128', 'Stalverket'),
             demo('Antwerpen (demo)', 'Antwerpen 110', 'Antwerpen')]

    with tempfile.TemporaryDirectory() as tmp:
        par = os.path.join(tmp, 'parametrar.txt')
        subprocess.run([kmrender, '--parametrar', par], check=True)
        index, intervall = {}, {}
        for rad in open(par, encoding='utf-8'):
            i, pid, lo, hi = rad.rstrip('\n').split('\t')
            index[pid] = int(i); intervall[pid] = (float(lo), float(hi))

        for lat in latar:
            lage = os.path.join(tmp, 'lage.xml')
            subprocess.run([kmrender, '--state', lat['preset'], lage] + lat['monster'], check=True)
            state_xml = open(lage, encoding='utf-8').read().strip()
            if lat['tempo'] is None:
                lat['tempo'], lat['langdSlag'] = las_lat(state_xml)

            namn = os.path.join(utkatalog, lat['fil'])
            with open(namn + '.RPP', 'w', encoding='utf-8') as f:
                f.write(gor_rpp(lat, state_xml, index, intervall, rid))
            spec = dict(lat, state=state_xml)
            with open(namn + '.json', 'w', encoding='utf-8') as f:
                json.dump(spec, f, ensure_ascii=False, indent=1)
            sek = lat['langdSlag'] * 60.0 / lat['tempo']
            print('%s: %d takter i %g BPM (%d:%02d), %d automationskurvor' % (
                namn + '.RPP', lat['langdSlag'] / 4, lat['tempo'], int(sek // 60), int(round(sek % 60)), len(lat['automation'])))
            if '--render' in sys.argv:
                subprocess.run([kmrender, '--projekt', namn + '.json', namn + '.wav'], check=True)

if __name__ == '__main__':
    main()
