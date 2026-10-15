"""UTAU oto.ini alias reader."""
def parse_oto_ini(oto_path):
    import os
    oto_data = {}
    encodings = ['utf-8-sig', 'shift_jis', 'gbk', 'cp437']
    lines = []
    for enc in encodings:
        try:
            with open(oto_path, 'r', encoding=enc) as f:
                lines = f.readlines()
            break
        except UnicodeDecodeError:
            continue
    for line in lines:
        line = line.strip()
        if not line or '=' not in line:
            continue
        try:
            wav_file, params_str = line.split('=', 1)
            parts = params_str.split(',')
            alias = parts[0].strip()
            default_alias = os.path.splitext(wav_file)[0]
            if not alias:
                alias = default_alias

            offset = float(parts[1]) if len(
                parts) > 1 and parts[1].strip() else 0.0
            consonant = float(parts[2]) if len(
                parts) > 2 and parts[2].strip() else 0.0
            cutoff = float(parts[3]) if len(
                parts) > 3 and parts[3].strip() else 0.0
            preutterance = float(parts[4]) if len(
                parts) > 4 and parts[4].strip() else 0.0
            overlap = float(parts[5]) if len(
                parts) > 5 and parts[5].strip() else 0.0

            entry_data = {
                'wav_file': wav_file, 'offset': offset, 'consonant': consonant,
                'cutoff': cutoff, 'preutterance': preutterance, 'overlap': overlap
            }
            oto_data[alias] = entry_data
            if default_alias not in oto_data:
                oto_data[default_alias] = entry_data
        except Exception:
            pass
    return oto_data


