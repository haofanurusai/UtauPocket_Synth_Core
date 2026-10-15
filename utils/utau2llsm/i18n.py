"""Shared keyed translations for the UI and background tasks."""
LANGUAGES = {'en': {'window_title': 'UTAU to LLSM2 Converter',
        'btn_browse': 'Browse...',
        'btn_convert': 'Convert',
        'lbl_utau_dir': 'UTAU Voicebank Dir',
        'lbl_csv_path': 'Syllable List',
        'lbl_out_dir': 'Output Directory',
        'lbl_fmin': 'Min Frequency',
        'lbl_fmax': 'Max Frequency',
        'lbl_trange': 'Max Transpose Semi-tones',
        'lbl_har': 'Max Harmonics',
        'lbl_har_e': 'Noise Env Harmonics',
        'lbl_bias': 'Voicing Bias',
        'lbl_hop': 'Hop Size',
        'lbl_fft': 'FFT Size',
        'lbl_psd': 'PSD Vector Size',
        'lbl_spec': 'Order for Spectrum Compression',
        'lbl_bap': 'Order for Band Aperiodicity Compression',
        'lbl_loop_start': 'Loop Start Pos (%)',
        'lbl_loop_end': 'Loop End Pos (%)',
        'err_title': 'Error',
        'err_exe_missing': 'wav2llsm not found. Build it or set UTAU_WAV2LLSM.',
        'warn_title': 'Warning: Missing Aliases',
        'contentvec_analysis': 'ContentVec analysis',
        'current': 'Current',
        'selected': 'Selected',
        'all': 'All',
        'choose_an_existing_model_file': 'Choose an existing model file.',
        'model_path': 'Model path',
        'try_gpu_fall_back_to_cpu': 'Try GPU (fall back to CPU)',
        'also_set_loop_bounds_replace_working_values_undoable': 'Also set loop bounds (replace '
                                                                'working values; undoable)',
        'analyzes_the_entire_cropped_wav_initial_model_loading_may_take_a_while': 'Analyzes the '
                                                                                  'entire cropped '
                                                                                  'WAV. Initial '
                                                                                  'model loading '
                                                                                  'may take a '
                                                                                  'while.',
        'invalid_parameters': 'Invalid parameters',
        'analyze': 'Analyze',
        'all_files': 'All files',
        'weights_are_used_as_entered_without_normalization_affect_automatic_bounds_only': 'Weights '
                                                                                          'are '
                                                                                          'used as '
                                                                                          'entered, '
                                                                                          'without '
                                                                                          'normalization; '
                                                                                          'affect '
                                                                                          'automatic '
                                                                                          'bounds '
                                                                                          'only.',
        'reset_all_loop_parameters': 'Reset all loop parameters',
        'relative_rms_threshold_must_be_below_1': 'Relative RMS threshold must be below 1',
        'feature_and_stability_weights_each_need_a_positive_member': 'Feature and stability '
                                                                     'weights each need a positive '
                                                                     'member',
        'enter_a_number': ': enter a number',
        'empty_wav_crop': 'Empty WAV crop',
        'unsupported_pcm_sample_width': 'Unsupported PCM sample width',
        'main_glide': 'Main / glide',
        'fm': 'FM',
        'am': 'AM',
        'speed': 'Speed',
        'voice': 'Voice',
        'attack_a': 'Attack A',
        'peak_p': 'Peak P',
        'decay_d': 'Decay D',
        'sustain_s': 'Sustain S',
        'release_r': 'Release R',
        'glide': 'Glide',
        'period': 'Period',
        'depth': 'Depth',
        'cc_consonant': 'Consonant',
        'vowel': 'Vowel',
        'volume': 'Volume',
        'nyquist_multiplier': 'Nyquist multiplier',
        'aperiodic_gain': 'Aperiodic gain',
        'openness_rd': 'Openness (Rd)',
        'lip_radius': 'Lip opening radius',
        'expected_one_finite_f0_rd_pair_per_frame': 'Expected one finite f0,rd pair per frame',
        'invalid_oto_timing_no_legal_loop_remains': 'Invalid oto timing; no legal loop remains',
        'output_directory_must_be_empty_choose_a_new_directory': 'Output directory must be empty; '
                                                                 'choose a new directory',
        'loop_percentages_must_satisfy_0_start_end_100': 'Loop percentages must satisfy 0 ≤ start '
                                                         '< end ≤ 100',
        'hop_fft_psd_spec_must_be_powers_of_two': 'hop/fft/psd/spec must be powers of two',
        'invalid_csv_row': ' (invalid CSV row)',
        'missing_alias_unsafe_or_duplicate_filename': ' (missing alias, unsafe or duplicate '
                                                      'filename)',
        'invalid_pe_file': 'Invalid PE file: ',
        'select_a_dll_not_an_exe_or_static_library': 'Select a DLL, not an EXE or static library: ',
        'dll_python_architecture_mismatch': 'DLL/Python architecture mismatch: ',
        'no_matching_shared_core_build_or_select_one': 'No matching shared core; build or select '
                                                       'one: ',
        'not_a_windows_pe_file': 'Not a Windows PE file: ',
        'cannot_load_core_check_core_exports_and_dependency_python_architecture': 'Cannot load '
                                                                                  'core; check '
                                                                                  'core exports '
                                                                                  'and '
                                                                                  'dependency/Python '
                                                                                  'architecture: ',
        'loading_contentvec': 'Loading ContentVec…',
        'closing_preview': 'Closing preview…',
        'saved_previous_database_oto_db_bak': 'Saved; previous database: oto.db.bak',
        'cancelled_completed_results_retained': 'Cancelled; completed results retained',
        'analysis_complete': 'Analysis complete',
        'cancellation_requested_close_after_current_inference_finishes': 'Cancellation requested; '
                                                                         'close after current '
                                                                         'inference finishes.',
        'unsaved_changes': 'Unsaved changes',
        'save_changes_to_oto_db': 'Save changes to oto.db?',
        'project': 'Project',
        'note': 'Note',
        'velocity': 'Velocity',
        'working': 'Working',
        'saved': 'Saved',
        'play': 'Play',
        'export_3s_wav': 'Export 3s WAV',
        'core_library': 'Core library…',
        'midi_note_0_127_velocity_1_127': 'MIDI note: 0–127; velocity: 1–127',
        'release': 'Release',
        'stop': 'Stop',
        'shared_library': 'Shared library',
        'wheel_to_zoom_drag_l_r_lines_to_edit_loop_frames': 'Wheel to zoom; drag L/R lines to edit '
                                                            'loop frames.',
        'save_oto_db': 'Save oto.db',
        'open_project': 'Open project…',
        'return_to_converter': 'Return to converter',
        'file': 'File',
        'undo_ctrl_z': 'Undo Ctrl+Z',
        'redo_ctrl_y': 'Redo Ctrl+Y',
        'edit': 'Edit',
        'contentvec_analysis_2': 'ContentVec analysis…',
        'cancel_analysis': 'Cancel analysis',
        'analysis': 'Analysis',
        'contentvec_grayscale': 'ContentVec grayscale',
        'fit_whole_clip': 'Fit whole clip',
        'view': 'View',
        'waveform': 'Waveform',
        'apply_frames': 'Apply frames',
        'oto_loop_editor': 'OTO loop editor — ',
        'saved_l_r': 'Saved L/R: ',
        'phonemes_unsaved': 'Phonemes · * unsaved',
        'integer_frames_con_l_r_nfrm': 'Integer frames · con ≤ L < R < nfrm',
        'project_wav_f0_rd_metadata_mismatch': 'Project WAV/F0/Rd metadata mismatch',
        'phoneme': 'Phoneme',
        'consonant_end': 'Consonant',
        'frames': 'Frames',
        'left': 'Left',
        'right': 'Right',
        'contentvec_must_return_finite_frames_768_features': 'ContentVec must return finite '
                                                             '[frames, 768] features',
        'cuda_failed_falling_back_to_cpu': 'CUDA failed; falling back to CPU: ',
        'unsupported_contentvec_convolution_geometry': 'Unsupported ContentVec convolution '
                                                       'geometry',
        'open_existing_project': 'Open project…',
        'converting': 'Converting',
        'please_wait_for_conversion': 'Please wait for conversion.',
        'check_input_paths': 'Check input paths.',
        '0_fmin_fmax': '0 < fmin < fmax',
        'settings': 'Settings',
        'invalid_loop_analysis_shapes_values_or_frame_parameters': 'Invalid loop analysis shapes, '
                                                                   'values or frame parameters',
        'no_continuous_valid_vowel_interval_meets_the_thresholds_and_minimum_duration': 'No '
                                                                                        'continuous '
                                                                                        'valid '
                                                                                        'vowel '
                                                                                        'interval '
                                                                                        'meets the '
                                                                                        'thresholds '
                                                                                        'and '
                                                                                        'minimum '
                                                                                        'duration',
        'analysis_cancelled': 'Analysis cancelled',
        'unknown_ls2_precision': 'Unknown LS2 precision',
        'non_finite_audio_from_core': 'Non-finite audio from core',
        'synthesis_failed': 'Synthesis failed: ',
        'core_init_failed_check_float_double_precision': 'Core init failed; check float/double '
                                                         'precision: ',
        'cannot_load_audio_output_check_sounddevice_portaudio_installation_and_architecture': 'Cannot '
                                                                                              'load '
                                                                                              'audio '
                                                                                              'output; '
                                                                                              'check '
                                                                                              'sounddevice/PortAudio '
                                                                                              'installation '
                                                                                              'and '
                                                                                              'architecture: ',
        '0_con_l_r_nfrm_65535': '0 ≤ con ≤ L < R < nfrm ≤ 65535',
        'unsupported_project_version': 'Unsupported project version',
        'invalid_oto_db_size': 'Invalid oto.db size',
        'project_asset_escapes_project_directory': 'Project asset escapes project directory',
        'oto_db_changed_externally_reopen_before_saving': 'oto.db changed externally; reopen '
                                                          'before saving',
        'analysis_min_loop_ms': 'Minimum loop (ms)',
        'analysis_min_f0_hz': 'Minimum F0 (Hz)',
        'analysis_min_rd': 'Rd lower bound (exclusive)',
        'analysis_power_threshold': 'Relative RMS threshold (0–1)',
        'analysis_tail_weight': 'Tail position weight',
        'analysis_cv_weight': 'ContentVec weight',
        'analysis_f0_weight': 'F0 weight',
        'analysis_rd_weight': 'Rd weight',
        'analysis_cv_scale': 'ContentVec variance scale',
        'analysis_f0_scale': 'F0 scale (semitones)',
        'analysis_rd_scale': 'Rd scale',
        'analysis_variance_weight': 'Interval variance weight',
        'analysis_rate_weight': 'Interval motion weight',
        'analysis_edge_weight': 'Endpoint motion weight',
        'analysis_length_weight': 'Length preference weight',
        'analysis_rate_reference_ms': 'Motion reference time (ms)',
        'analysis_edge_window_ms': 'Endpoint window (ms)',
        'must_be_finite_and_positive': ': must be finite and positive',
        'must_be_finite_and_nonnegative': ': must be finite and nonnegative',
        'initial_loops_adjusted': 'Initial loops adjusted to legal bounds for {count} entries.',
        'menu_language': 'Language',
        'analysis_group_bounds': 'Constraints',
        'analysis_group_features': 'Feature metric',
        'analysis_group_objective': 'Objective'},
 'zh_CN': {'window_title': 'UTAU音源转LLSM2工具',
           'btn_browse': '浏览...',
           'btn_convert': '开始转换',
           'lbl_utau_dir': '音源目录',
           'lbl_csv_path': '音节清单',
           'lbl_out_dir': '输出目录',
           'lbl_fmin': '基频最小值',
           'lbl_fmax': '基频最大值',
           'lbl_trange': '最大移调半音',
           'lbl_har': '最大谐波数量',
           'lbl_har_e': '噪声包络最大谐波数',
           'lbl_bias': '发声/浊音偏置',
           'lbl_hop': '帧移大小',
           'lbl_fft': 'FFT窗口大小',
           'lbl_psd': '功率谱密度向量大小',
           'lbl_spec': '频谱压缩阶数',
           'lbl_bap': '频带非周期性压缩阶数',
           'lbl_loop_start': '元音起始位置百分比 (%)',
           'lbl_loop_end': '元音终止位置百分比 (%)',
           'err_title': '错误',
           'err_exe_missing': '未找到 wav2llsm；请先构建或设置 UTAU_WAV2LLSM。',
           'warn_title': '警告：部分别名转换失败',
           'contentvec_analysis': 'ContentVec 分析',
           'current': '当前音素',
           'selected': '选中音素',
           'all': '全部音素',
           'choose_an_existing_model_file': '请选择有效模型文件。',
           'model_path': '模型路径',
           'try_gpu_fall_back_to_cpu': '尝试使用GPU（失败后回退CPU）',
           'also_set_loop_bounds_replace_working_values_undoable': '分析时一并自动定界（覆盖工作值，可撤销）',
           'analyzes_the_entire_cropped_wav_initial_model_loading_may_take_a_while': '分析整段裁剪 '
                                                                                     'WAV；首次加载模型可能较慢。',
           'invalid_parameters': '参数错误',
           'analyze': '开始分析',
           'all_files': '所有文件',
           'weights_are_used_as_entered_without_normalization_affect_automatic_bounds_only': '权重按原值使用，不自动归一化；仅勾选自动定界时影响边界。',
           'reset_all_loop_parameters': '恢复全部定界默认值',
           'relative_rms_threshold_must_be_below_1': '相对 RMS 下限必须小于 1',
           'feature_and_stability_weights_each_need_a_positive_member': '特征权重和稳定性权重各至少有一项大于零',
           'enter_a_number': '：请输入数值',
           'empty_wav_crop': '裁剪后的 WAV 为空',
           'unsupported_pcm_sample_width': '不支持此 PCM 采样位宽',
           'main_glide': '主包络 / 滑音',
           'fm': '频率调制',
           'am': '振幅调制',
           'speed': '播放速度',
           'voice': '音色',
           'attack_a': '起音 A',
           'peak_p': '峰值 P',
           'decay_d': '衰减 D',
           'sustain_s': '保持 S',
           'release_r': '释放 R',
           'glide': '滑音',
           'period': '周期',
           'depth': '深度',
           'cc_consonant': '辅音',
           'vowel': '元音',
           'volume': '总音量',
           'nyquist_multiplier': '奈奎斯特倍率',
           'aperiodic_gain': '非周期分量增益',
           'openness_rd': '开口度（Rd）',
           'lip_radius': '唇部开口半径',
           'expected_one_finite_f0_rd_pair_per_frame': 'wav2llsm 必须每行输出一帧的 f0,rd，且值为有限数值',
           'invalid_oto_timing_no_legal_loop_remains': 'oto 时序参数无效，没有可用的循环范围',
           'output_directory_must_be_empty_choose_a_new_directory': '输出目录必须为空，请选择新目录',
           'loop_percentages_must_satisfy_0_start_end_100': '循环百分比必须满足 0 ≤ 起点 < 终点 ≤ 100',
           'hop_fft_psd_spec_must_be_powers_of_two': '帧移、FFT、PSD 和频谱压缩阶数必须为 2 的整数幂',
           'invalid_csv_row': '（CSV 行格式错误）',
           'missing_alias_unsafe_or_duplicate_filename': '（别名不存在、文件名不安全或重复）',
           'invalid_pe_file': '无效 PE 文件：',
           'select_a_dll_not_an_exe_or_static_library': '请选择动态库 DLL，不能使用 EXE 或静态库：',
           'dll_python_architecture_mismatch': 'DLL 与 Python 架构不匹配：',
           'no_matching_shared_core_build_or_select_one': '未找到匹配的合成动态库，请构建或手动选择：',
           'not_a_windows_pe_file': '不是 Windows PE 文件：',
           'cannot_load_core_check_core_exports_and_dependency_python_architecture': '无法加载合成内核；请确认是本项目 '
                                                                                     'DLL，且其依赖与 '
                                                                                     'Python 架构一致：',
           'loading_contentvec': '加载 ContentVec…',
           'closing_preview': '正在关闭试听…',
           'saved_previous_database_oto_db_bak': '已保存；原数据库备份为 oto.db.bak',
           'cancelled_completed_results_retained': '已取消；已完成结果已保留',
           'analysis_complete': '分析完成',
           'cancellation_requested_close_after_current_inference_finishes': '已请求取消；请等当前推理结束后关闭。',
           'unsaved_changes': '未保存修改',
           'save_changes_to_oto_db': '保存 oto.db 的修改？',
           'project': '项目',
           'note': '音高',
           'velocity': '力度',
           'working': '工作值',
           'saved': '已保存',
           'play': '试听',
           'export_3s_wav': '导出3秒WAV',
           'core_library': '选择内核库…',
           'midi_note_0_127_velocity_1_127': 'MIDI 音高范围为 0–127；力度范围为 1–127',
           'release': '释放',
           'stop': '停止',
           'shared_library': '动态链接库',
           'wheel_to_zoom_drag_l_r_lines_to_edit_loop_frames': '滚轮缩放；拖动 L/R 线调整循环帧。',
           'save_oto_db': '保存 oto.db',
           'open_project': '打开项目…',
           'return_to_converter': '返回转换器',
           'file': '文件',
           'undo_ctrl_z': '撤销 Ctrl+Z',
           'redo_ctrl_y': '重做 Ctrl+Y',
           'edit': '编辑',
           'contentvec_analysis_2': 'ContentVec 分析…',
           'cancel_analysis': '取消分析',
           'analysis': '分析',
           'contentvec_grayscale': 'ContentVec 灰度图',
           'fit_whole_clip': '适应整段',
           'view': '视图',
           'waveform': '波形',
           'apply_frames': '应用帧索引',
           'oto_loop_editor': 'OTO 循环细调 — ',
           'saved_l_r': '已保存 L/R: ',
           'phonemes_unsaved': '音素 · * 为未保存',
           'integer_frames_con_l_r_nfrm': '整数帧 · con ≤ L < R < nfrm',
           'project_wav_f0_rd_metadata_mismatch': '项目 WAV、F0、Rd 数据与元信息不匹配',
           'phoneme': '音素',
           'consonant_end': '辅音终点',
           'frames': '总帧数',
           'left': '左边界',
           'right': '右边界',
           'contentvec_must_return_finite_frames_768_features': 'ContentVec 必须返回无无穷值或非数值的 [帧数, '
                                                                '768] 特征',
           'cuda_failed_falling_back_to_cpu': 'CUDA 失败，回退 CPU：',
           'unsupported_contentvec_convolution_geometry': '不支持此 ContentVec 卷积结构',
           'open_existing_project': '打开已有项目…',
           'converting': '正在转换',
           'please_wait_for_conversion': '请等待转换结束。',
           'check_input_paths': '请检查输入路径。',
           '0_fmin_fmax': '基频范围必须满足：0 < 最小基频 < 最大基频',
           'settings': '设置',
           'invalid_loop_analysis_shapes_values_or_frame_parameters': '自动定界输入的形状、数值或帧参数无效',
           'no_continuous_valid_vowel_interval_meets_the_thresholds_and_minimum_duration': '没有满足当前阈值及最短长度的连续有效元音区段',
           'analysis_cancelled': '分析已取消',
           'unknown_ls2_precision': '未知的 LS2 浮点精度',
           'non_finite_audio_from_core': '合成内核返回了无穷值或非数值音频',
           'synthesis_failed': '合成失败：',
           'core_init_failed_check_float_double_precision': '内核初始化失败，请检查 float/double 库与声库精度是否一致：',
           'cannot_load_audio_output_check_sounddevice_portaudio_installation_and_architecture': '无法加载音频输出模块，请检查 '
                                                                                                 'sounddevice/PortAudio '
                                                                                                 '安装及其架构：',
           '0_con_l_r_nfrm_65535': '帧索引必须满足：0 ≤ 辅音终点 ≤ 左边界 < 右边界 < 总帧数 ≤ 65535',
           'unsupported_project_version': '不支持的项目版本',
           'invalid_oto_db_size': 'oto.db 文件大小不合法',
           'project_asset_escapes_project_directory': '项目资源路径超出项目目录',
           'oto_db_changed_externally_reopen_before_saving': 'oto.db 已被外部程序修改，请重新打开后再保存',
           'analysis_min_loop_ms': '最短循环长度 (ms)',
           'analysis_min_f0_hz': '最低 F0 (Hz)',
           'analysis_min_rd': 'Rd 下限（严格大于）',
           'analysis_power_threshold': '相对 RMS 下限 (0–1)',
           'analysis_tail_weight': '尾部位置权重',
           'analysis_cv_weight': 'ContentVec 权重',
           'analysis_f0_weight': 'F0 权重',
           'analysis_rd_weight': 'Rd 权重',
           'analysis_cv_scale': 'ContentVec 方差尺度',
           'analysis_f0_scale': 'F0 尺度（半音）',
           'analysis_rd_scale': 'Rd 尺度',
           'analysis_variance_weight': '区间离散度权重',
           'analysis_rate_weight': '区间变化率权重',
           'analysis_edge_weight': '折返点变化率权重',
           'analysis_length_weight': '长度偏好权重',
           'analysis_rate_reference_ms': '变化率参考时间 (ms)',
           'analysis_edge_window_ms': '折返点窗口 (ms)',
           'must_be_finite_and_positive': '：必须是有限正数',
           'must_be_finite_and_nonnegative': '：必须是有限非负数',
           'initial_loops_adjusted': '{count} 个音素的初始循环已修正为合法范围。',
           'menu_language': '语言',
           'analysis_group_bounds': '候选约束',
           'analysis_group_features': '特征权重与尺度',
           'analysis_group_objective': '目标函数'},
 'es': {'window_title': 'Conversor de UTAU a LLSM2',
        'btn_browse': 'Examinar...',
        'btn_convert': 'Convertir',
        'lbl_utau_dir': 'Directorio del banco de voz UTAU',
        'lbl_csv_path': 'Lista de sílabas',
        'lbl_out_dir': 'Directorio de salida',
        'lbl_fmin': 'Frecuencia mínima',
        'lbl_fmax': 'Frecuencia máxima',
        'lbl_trange': 'Transposición máxima (semitonos)',
        'lbl_har': 'Número máximo de armónicos',
        'lbl_har_e': 'Armónicos de la envolvente de ruido',
        'lbl_bias': 'Sesgo de sonoridad',
        'lbl_hop': 'Paso entre tramas',
        'lbl_fft': 'Tamaño de FFT',
        'lbl_psd': 'Tamaño del vector de densidad espectral de potencia',
        'lbl_spec': 'Orden de compresión del espectro',
        'lbl_bap': 'Orden de compresión de aperiodicidad por bandas',
        'lbl_loop_start': 'Inicio del bucle (%)',
        'lbl_loop_end': 'Fin del bucle (%)',
        'err_title': 'Error',
        'err_exe_missing': 'No se encontró wav2llsm. Compílelo o configure UTAU_WAV2LLSM.',
        'warn_title': 'Advertencia: algunos alias no se pudieron convertir',
        'contentvec_analysis': 'Análisis de ContentVec',
        'current': 'Fonema actual',
        'selected': 'Fonemas seleccionados',
        'all': 'Todos los fonemas',
        'choose_an_existing_model_file': 'Seleccione un archivo de modelo existente.',
        'model_path': 'Ruta del modelo',
        'try_gpu_fall_back_to_cpu': 'Intentar usar la GPU (usar la CPU si falla)',
        'also_set_loop_bounds_replace_working_values_undoable': 'Ajustar también los límites del '
                                                                'bucle (sustituye los valores de '
                                                                'trabajo; se puede deshacer)',
        'analyzes_the_entire_cropped_wav_initial_model_loading_may_take_a_while': 'Se analiza todo '
                                                                                  'el WAV '
                                                                                  'recortado. La '
                                                                                  'primera carga '
                                                                                  'del modelo '
                                                                                  'puede tardar.',
        'invalid_parameters': 'Parámetros no válidos',
        'analyze': 'Analizar',
        'all_files': 'Todos los archivos',
        'weights_are_used_as_entered_without_normalization_affect_automatic_bounds_only': 'Los '
                                                                                          'pesos '
                                                                                          'se usan '
                                                                                          'sin '
                                                                                          'normalizar; '
                                                                                          'solo '
                                                                                          'afectan '
                                                                                          'a la '
                                                                                          'delimitación '
                                                                                          'automática.',
        'reset_all_loop_parameters': 'Restablecer todos los parámetros del bucle',
        'relative_rms_threshold_must_be_below_1': 'El umbral RMS relativo debe ser menor que 1',
        'feature_and_stability_weights_each_need_a_positive_member': 'Los grupos de pesos de '
                                                                     'características y de '
                                                                     'estabilidad deben tener al '
                                                                     'menos un valor positivo cada '
                                                                     'uno',
        'enter_a_number': ': introduzca un número',
        'empty_wav_crop': 'El WAV recortado está vacío',
        'unsupported_pcm_sample_width': 'Profundidad de bits PCM no compatible',
        'main_glide': 'Envolvente principal / portamento',
        'fm': 'Modulación de frecuencia',
        'am': 'Modulación de amplitud',
        'speed': 'Velocidad de reproducción',
        'voice': 'Timbre',
        'attack_a': 'Ataque A',
        'peak_p': 'Pico P',
        'decay_d': 'Decaimiento D',
        'sustain_s': 'Sostenimiento S',
        'release_r': 'Liberación R',
        'glide': 'Portamento',
        'period': 'Periodo',
        'depth': 'Profundidad',
        'cc_consonant': 'Consonante',
        'vowel': 'Vocal',
        'volume': 'Volumen general',
        'nyquist_multiplier': 'Multiplicador de Nyquist',
        'aperiodic_gain': 'Ganancia aperiódica',
        'openness_rd': 'Apertura (Rd)',
        'lip_radius': 'Radio de apertura de los labios',
        'expected_one_finite_f0_rd_pair_per_frame': 'wav2llsm debe devolver un par f0,rd de '
                                                    'valores finitos por trama',
        'invalid_oto_timing_no_legal_loop_remains': 'Tiempos oto no válidos; no queda ningún '
                                                    'intervalo de bucle válido',
        'output_directory_must_be_empty_choose_a_new_directory': 'El directorio de salida debe '
                                                                 'estar vacío; seleccione uno '
                                                                 'nuevo',
        'loop_percentages_must_satisfy_0_start_end_100': 'Los porcentajes del bucle deben cumplir '
                                                         '0 ≤ inicio < fin ≤ 100',
        'hop_fft_psd_spec_must_be_powers_of_two': 'hop/fft/psd/spec deben ser potencias de dos',
        'invalid_csv_row': ' (fila CSV no válida)',
        'missing_alias_unsafe_or_duplicate_filename': ' (alias inexistente o nombre de archivo no '
                                                      'seguro o duplicado)',
        'invalid_pe_file': 'Archivo PE no válido: ',
        'select_a_dll_not_an_exe_or_static_library': 'Seleccione una DLL, no un EXE ni una '
                                                     'biblioteca estática: ',
        'dll_python_architecture_mismatch': 'La arquitectura de la DLL no coincide con la de '
                                            'Python: ',
        'no_matching_shared_core_build_or_select_one': 'No se encontró una biblioteca de síntesis '
                                                       'compatible; compílela o seleccione una: ',
        'not_a_windows_pe_file': 'No es un archivo PE de Windows: ',
        'cannot_load_core_check_core_exports_and_dependency_python_architecture': 'No se puede '
                                                                                  'cargar el '
                                                                                  'motor; '
                                                                                  'compruebe las '
                                                                                  'funciones '
                                                                                  'exportadas y la '
                                                                                  'arquitectura de '
                                                                                  'las '
                                                                                  'dependencias y '
                                                                                  'de Python: ',
        'loading_contentvec': 'Cargando ContentVec…',
        'closing_preview': 'Cerrando la preescucha…',
        'saved_previous_database_oto_db_bak': 'Guardado; copia de la base de datos anterior: '
                                              'oto.db.bak',
        'cancelled_completed_results_retained': 'Cancelado; se conservan los resultados '
                                                'completados',
        'analysis_complete': 'Análisis completado',
        'cancellation_requested_close_after_current_inference_finishes': 'Cancelación solicitada; '
                                                                         'cierre cuando termine la '
                                                                         'inferencia actual.',
        'unsaved_changes': 'Cambios sin guardar',
        'save_changes_to_oto_db': '¿Guardar los cambios en oto.db?',
        'project': 'Proyecto',
        'note': 'Nota',
        'velocity': 'Velocidad MIDI',
        'working': 'Valores de trabajo',
        'saved': 'Valores guardados',
        'play': 'Escuchar',
        'export_3s_wav': 'Exportar WAV de 3 segundos',
        'core_library': 'Biblioteca de síntesis…',
        'midi_note_0_127_velocity_1_127': 'Nota MIDI: 0–127; velocidad: 1–127',
        'release': 'Liberar',
        'stop': 'Detener',
        'shared_library': 'Biblioteca dinámica',
        'wheel_to_zoom_drag_l_r_lines_to_edit_loop_frames': 'Use la rueda para ampliar; arrastre '
                                                            'las líneas L/R para ajustar el bucle.',
        'save_oto_db': 'Guardar oto.db',
        'open_project': 'Abrir proyecto…',
        'return_to_converter': 'Volver al conversor',
        'file': 'Archivo',
        'undo_ctrl_z': 'Deshacer Ctrl+Z',
        'redo_ctrl_y': 'Rehacer Ctrl+Y',
        'edit': 'Editar',
        'contentvec_analysis_2': 'Análisis de ContentVec…',
        'cancel_analysis': 'Cancelar análisis',
        'analysis': 'Análisis',
        'contentvec_grayscale': 'ContentVec en escala de grises',
        'fit_whole_clip': 'Ajustar al fragmento completo',
        'view': 'Ver',
        'waveform': 'Forma de onda',
        'apply_frames': 'Aplicar índices de trama',
        'oto_loop_editor': 'Editor de bucles OTO — ',
        'saved_l_r': 'L/R guardados: ',
        'phonemes_unsaved': 'Fonemas · * sin guardar',
        'integer_frames_con_l_r_nfrm': 'Índices enteros · con ≤ L < R < nfrm',
        'project_wav_f0_rd_metadata_mismatch': 'Los datos WAV/F0/Rd no coinciden con los metadatos '
                                               'del proyecto',
        'phoneme': 'Fonema',
        'consonant_end': 'Fin de la consonante',
        'frames': 'Tramas',
        'left': 'Límite izquierdo',
        'right': 'Límite derecho',
        'contentvec_must_return_finite_frames_768_features': 'ContentVec debe devolver '
                                                             'características finitas de forma '
                                                             '[tramas, 768]',
        'cuda_failed_falling_back_to_cpu': 'CUDA falló; se usará la CPU: ',
        'unsupported_contentvec_convolution_geometry': 'Estructura de convolución de ContentVec no '
                                                       'compatible',
        'open_existing_project': 'Abrir proyecto…',
        'converting': 'Convirtiendo',
        'please_wait_for_conversion': 'Espere a que termine la conversión.',
        'check_input_paths': 'Compruebe las rutas de entrada.',
        '0_fmin_fmax': 'El rango de frecuencia fundamental debe cumplir 0 < fmin < fmax',
        'settings': 'Configuración',
        'invalid_loop_analysis_shapes_values_or_frame_parameters': 'Dimensiones, valores o '
                                                                   'parámetros de trama no válidos '
                                                                   'para el análisis del bucle',
        'no_continuous_valid_vowel_interval_meets_the_thresholds_and_minimum_duration': 'Ningún '
                                                                                        'intervalo '
                                                                                        'continuo '
                                                                                        'de vocal '
                                                                                        'válido '
                                                                                        'cumple '
                                                                                        'los '
                                                                                        'umbrales '
                                                                                        'y la '
                                                                                        'duración '
                                                                                        'mínima',
        'analysis_cancelled': 'Análisis cancelado',
        'unknown_ls2_precision': 'Precisión LS2 desconocida',
        'non_finite_audio_from_core': 'El motor de síntesis devolvió audio con valores no finitos',
        'synthesis_failed': 'La síntesis falló: ',
        'core_init_failed_check_float_double_precision': 'La inicialización del motor falló; '
                                                         'compruebe la precisión float/double: ',
        'cannot_load_audio_output_check_sounddevice_portaudio_installation_and_architecture': 'No '
                                                                                              'se '
                                                                                              'puede '
                                                                                              'cargar '
                                                                                              'la '
                                                                                              'salida '
                                                                                              'de '
                                                                                              'audio; '
                                                                                              'compruebe '
                                                                                              'la '
                                                                                              'instalación '
                                                                                              'y '
                                                                                              'la '
                                                                                              'arquitectura '
                                                                                              'de '
                                                                                              'sounddevice/PortAudio: ',
        '0_con_l_r_nfrm_65535': 'Los índices de trama deben cumplir 0 ≤ con ≤ L < R < nfrm ≤ 65535',
        'unsupported_project_version': 'Versión del proyecto no compatible',
        'invalid_oto_db_size': 'Tamaño de oto.db no válido',
        'project_asset_escapes_project_directory': 'La ruta de un recurso está fuera del '
                                                   'directorio del proyecto',
        'oto_db_changed_externally_reopen_before_saving': 'Otro programa modificó oto.db; vuelva a '
                                                          'abrirlo antes de guardar',
        'analysis_min_loop_ms': 'Duración mínima del bucle (ms)',
        'analysis_min_f0_hz': 'F0 mínima (Hz)',
        'analysis_min_rd': 'Límite inferior de Rd (exclusivo)',
        'analysis_power_threshold': 'Umbral RMS relativo (0–1)',
        'analysis_tail_weight': 'Peso de la posición final',
        'analysis_cv_weight': 'Peso de ContentVec',
        'analysis_f0_weight': 'Peso de F0',
        'analysis_rd_weight': 'Peso de Rd',
        'analysis_cv_scale': 'Escala de varianza de ContentVec',
        'analysis_f0_scale': 'Escala de F0 (semitonos)',
        'analysis_rd_scale': 'Escala de Rd',
        'analysis_variance_weight': 'Peso de la dispersión del intervalo',
        'analysis_rate_weight': 'Peso de la tasa de cambio del intervalo',
        'analysis_edge_weight': 'Peso de la tasa de cambio en los extremos',
        'analysis_length_weight': 'Peso de la preferencia de duración',
        'analysis_rate_reference_ms': 'Tiempo de referencia para la tasa de cambio (ms)',
        'analysis_edge_window_ms': 'Ventana de los extremos (ms)',
        'must_be_finite_and_positive': ': debe ser un número finito y positivo',
        'must_be_finite_and_nonnegative': ': debe ser un número finito y no negativo',
        'initial_loops_adjusted': 'Se ajustaron los bucles iniciales de {count} fonemas a límites '
                                  'válidos.',
        'menu_language': 'Idioma',
        'analysis_group_bounds': 'Restricciones',
        'analysis_group_features': 'Pesos y escalas de características',
        'analysis_group_objective': 'Función objetivo'}}

_texts = LANGUAGES['zh_CN']


def get(key):
    """Look up a message in the application's active language."""
    return _texts.get(key, key)


class I18N:
    def __init__(self, lang="zh_CN"):
        normalized = lang.replace('-', '_').lower()
        if normalized == 'zh' or normalized.startswith('zh_'):
            self.lang = 'zh_CN'
        elif normalized == 'es' or normalized.startswith('es_'):
            self.lang = 'es'
        elif normalized == 'en' or normalized.startswith('en_'):
            self.lang = 'en'
        else:
            self.lang = 'en'
        self.texts = LANGUAGES[self.lang]
        global _texts
        _texts = self.texts

    def get(self, key):
        return self.texts.get(key, key)
