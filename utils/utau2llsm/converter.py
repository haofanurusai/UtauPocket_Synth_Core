"""Conversion orchestration. ML analysis belongs to the editor, not conversion."""
from i18n import get
import csv
import os
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from oto_ini import parse_oto_ini
from conversion_task import convert_one
from project import atomic_write, write_json


def check_power_of_2(value):
    try:
        v = int(value)
        return v > 0 and v & (v - 1) == 0
    except (ValueError, TypeError):
        return False


def run_conversion(exe_paths, utau_dir, csv_path, out_dir, params, progress_cb=None):
    output = Path(out_dir)
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise ValueError(get('output_directory_must_be_empty_choose_a_new_directory'))
    if not 0 <= params['loop_start_pct'] < params['loop_end_pct'] <= 100:
        raise ValueError(get('loop_percentages_must_satisfy_0_start_end_100'))
    if any(not check_power_of_2(params[k]) for k in ('hop','fft','psd','spec')):
        raise ValueError(get('hop_fft_psd_spec_must_be_powers_of_two'))
    oto = parse_oto_ini(Path(utau_dir) / 'oto.ini')
    tasks, failures, seen = [], [], set()
    with open(csv_path, encoding='utf-8-sig', newline='') as stream:
        reader = csv.reader(stream)
        next(reader, None)
        for row in reader:
            if not row or not any(row):
                continue
            if len(row) < 3:
                failures.append(str(row) + get('invalid_csv_row'))
                continue
            alias, bank, program = (s.strip() for s in row[:3])
            valid = all(len(s) == 1 and 33 <= ord(s) <= 126 and s not in '/\\:*?"<>|'
                        for s in (bank, program))
            if not valid or (bank+program).casefold() in seen or alias not in oto:
                failures.append(alias + get('missing_alias_unsafe_or_duplicate_filename'))
                continue
            seen.add((bank+program).casefold())
            tasks.append((alias, bank, program, oto[alias]))
    if not tasks:
        return 0, failures
    results = {}
    with ThreadPoolExecutor(max_workers=min(8, len(tasks), max(1, (os.cpu_count() or 2)-1))) as pool:
        futures = {pool.submit(convert_one, exe_paths['wav2llsm'], utau_dir, output, params, task): i
                   for i, task in enumerate(tasks)}
        for done, future in enumerate(as_completed(futures), 1):
            index = futures[future]
            try:
                results[index] = future.result()
            except Exception as error:
                failures.append(tasks[index][0] + ': ' + str(error))
            if progress_cb:
                progress_cb(done, len(tasks))
    ordered = [results[i] for i in sorted(results)]
    if ordered:
        atomic_write(output / 'oto.db', b''.join(row for row, _ in ordered))
        write_json(output / 'project.json', dict(version=1, database='oto.db',
                   parameters=params, entries=[meta for _, meta in ordered], failures=failures))
    return len(ordered), failures
