#!/usr/bin/env python3

import os
import sys
import logging
import traceback
import threading
import queue
from tkinterdnd2 import TkinterDnD, DND_FILES
from tkinter import (
    Tk, Label, Entry, Button, StringVar, IntVar, BooleanVar, filedialog,
    messagebox, Frame, Checkbutton, Radiobutton, LabelFrame 
)
from tkinter import ttk
from pydub import AudioSegment
from pydub.utils import which
import subprocess
import json

def toggle_sample_rate_dropdown():
    if override_sample_rate_var.get():
        sample_rate_dropdown.config(state="readonly")
    else:
        sample_rate_dropdown.config(state="disabled")

def toggle_bit_depth_dropdown():
    if override_bit_depth_var.get():
        bit_depth_dropdown.config(state="readonly")
    else:
        bit_depth_dropdown.config(state="disabled")

def get_application_root():
    if getattr(sys, 'frozen', False):
        # If the application is frozen, return the temporary folder where it's extracted
        app_root = sys._MEIPASS
    else:
        app_root = os.path.dirname(os.path.abspath(__file__))
    return app_root

tkdnd_path = os.path.join(get_application_root(), "tkdnd")
if tkdnd_path not in sys.path:
    sys.path.append(tkdnd_path)

def get_log_file_path():
    home = os.path.expanduser("~")
    if sys.platform == 'darwin':
        log_dir = os.path.join(home, "Library", "Logs", "ZQSFXAudioSplitter")
    elif sys.platform == 'win32':
        log_dir = os.path.join(home, "AppData", "Local", "ZQSFXAudioSplitter", "Logs")
    else:
        log_dir = os.path.join(home, ".ZQSFXAudioSplitter", "logs")
    os.makedirs(log_dir, exist_ok=True)
    return os.path.join(log_dir, "app.log")

def setup_logging():
    try:
        log_file_path = get_log_file_path()
        logging.basicConfig(
            level=logging.DEBUG,
            format="%(asctime)s - %(levelname)s - %(message)s",
            handlers=[
                logging.FileHandler(log_file_path),
                logging.StreamHandler(sys.stdout),
            ],
        )
    except Exception as e:
        logging.basicConfig(
            level=logging.DEBUG,
            format="%(asctime)s - %(levelname)s - %(message)s",
            handlers=[logging.StreamHandler(sys.stdout)],
        )
        logging.error(f"Failed to set up logging to file: {e}")

def handle_drop(event, dir_var, message_queue):
    try:
        # Extract the dropped path
        dropped_path = event.data.strip('{}')
        logger.debug(f"Dropped path: {dropped_path}")

        # Check if the dropped path is a directory
        if os.path.isdir(dropped_path):
            dir_var.set(dropped_path)
            logger.debug(f"Directory set via drag-and-drop: {dropped_path}")
            update_file_count()
        else:
            logger.error(f"Dropped item is not a directory: {dropped_path}")
            message_queue.put(("error", "Error", "Dropped item is not a directory."))
    except Exception as e:
        logger.error(f"Error handling drag-and-drop: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error handling drag-and-drop: {e}"))

setup_logging()
logger = logging.getLogger(__name__)

last_input_dir = os.path.expanduser("~")
last_output_dir = os.path.expanduser("~")

def get_ffmpeg_paths():
    app_root = get_application_root()

    if os.name == "nt":
        ffmpeg_filename = "ffmpeg.exe"
        ffprobe_filename = "ffprobe.exe"
    else:
        ffmpeg_filename = "ffmpeg"
        ffprobe_filename = "ffprobe"

    # Paths to ffmpeg and ffprobe in the 'ffmpeg' subdirectory
    ffmpeg_path = os.path.join(app_root, 'ffmpeg', ffmpeg_filename)
    ffprobe_path = os.path.join(app_root, 'ffmpeg', ffprobe_filename)

    # Check if FFmpeg binaries exist in the subdirectory
    if not os.path.exists(ffmpeg_path):
        logger.debug(f"FFmpeg not found in '{ffmpeg_path}'. Searching in system PATH.")
        ffmpeg_path = which("ffmpeg")
    if not os.path.exists(ffprobe_path):
        logger.debug(f"FFprobe not found in '{ffprobe_path}'. Searching in system PATH.")
        ffprobe_path = which("ffprobe")

    logger.debug(f"FFmpeg Path: {ffmpeg_path}")
    logger.debug(f"FFprobe Path: {ffprobe_path}")

    if ffmpeg_path and ffprobe_path and os.path.exists(ffmpeg_path) and os.path.exists(ffprobe_path):
        logger.info(f"Using FFmpeg at: {ffmpeg_path}")
        logger.info(f"Using FFprobe at: {ffprobe_path}")
        return ffmpeg_path, ffprobe_path
    else:
        logger.critical("FFmpeg and/or FFprobe not found. The application will exit.")
        messagebox.showerror("Error", "FFmpeg and/or FFprobe not found. Please ensure they are installed and included with the application.")
        sys.exit(1)

def get_bits_per_sample(file_path, ffprobe_path):
    try:
        cmd = [
            ffprobe_path,
            "-v", "error",
            "-select_streams", "a:0",
            "-show_entries", "stream=bits_per_sample",
            "-of", "default=noprint_wrappers=1:nokey=1",
            file_path,
        ]
        output = subprocess.check_output(cmd).decode().strip()
        bits_per_sample = int(output)
        logger.debug(f"Bits per sample for '{file_path}': {bits_per_sample}")
        return bits_per_sample
    except subprocess.CalledProcessError as e:
        logger.error(f"FFprobe error for '{file_path}': {e}")
        logger.debug(traceback.format_exc())
        return None
    except Exception as e:
        logger.error(f"Error getting bits per sample for '{file_path}': {e}")
        logger.debug(traceback.format_exc())
        return None

def get_sample_fmt(bits_per_sample):
    mapping = {8: "u8", 16: "s16", 24: "s24", 32: "s32"}
    sample_fmt = mapping.get(bits_per_sample)
    if sample_fmt is None:
        logger.error(f"Unsupported bits per sample: {bits_per_sample}")
    else:
        logger.debug(f"Mapped bits_per_sample {bits_per_sample} to sample_fmt {sample_fmt}")
    return sample_fmt

def get_metadata(file_path, ffprobe_path):
    try:
        cmd = [
            ffprobe_path,
            "-v", "quiet",
            "-print_format", "json",
            "-show_format",
            "-show_streams",
            file_path,
        ]
        output = subprocess.check_output(cmd).decode()
        data = json.loads(output)
        metadata = data.get("format", {}).get("tags", {})
        audio_stream = next((stream for stream in data.get("streams", []) if stream["codec_type"] == "audio"), None)
        if audio_stream:
            metadata["channels"] = audio_stream.get("channels", 0)
            metadata["sample_rate"] = audio_stream.get("sample_rate", "0")
        logger.debug(f"Extracted metadata for '{file_path}': {metadata}")
        return metadata
    except subprocess.CalledProcessError as e:
        logger.error(f"FFprobe error for '{file_path}': {e}")
        logger.debug(traceback.format_exc())
        return {}
    except Exception as e:
        logger.error(f"Error extracting metadata from '{file_path}': {e}")
        logger.debug(traceback.format_exc())
        return {}

ffmpeg_path, ffprobe_path = get_ffmpeg_paths()

# Set AudioSegment converter and ffprobe
AudioSegment.converter = ffmpeg_path
AudioSegment.ffprobe = ffprobe_path
logger.debug(f"AudioSegment.ffprobe set to: {AudioSegment.ffprobe}")

# **Add the ffmpeg directory to PATH**
ffmpeg_dir = os.path.dirname(ffmpeg_path)
if ffmpeg_dir not in os.environ["PATH"]:
    os.environ["PATH"] += os.pathsep + ffmpeg_dir
    logger.debug(f"Updated PATH environment variable with ffmpeg directory: {ffmpeg_dir}")

def split_audio_files(input_dir, output_dir, progress_var, progress_bar, total_files, message_queue, ffprobe_path, override_sample_rate, override_bit_depth, selected_channels):
    try:
        # Ensure ffprobe path is set within the thread
        AudioSegment.ffprobe = ffprobe_path
        logger.debug(f"AudioSegment.ffprobe within thread set to: {AudioSegment.ffprobe}")

        # Ensure ffmpeg directory is in PATH within the thread
        ffmpeg_dir = os.path.dirname(ffprobe_path)
        if ffmpeg_dir not in os.environ["PATH"]:
            os.environ["PATH"] += os.pathsep + ffmpeg_dir
            logger.debug(f"Thread: Updated PATH environment variable with ffmpeg directory: {ffmpeg_dir}")

        if not os.path.isdir(input_dir):
            logger.error(f"Input directory '{input_dir}' does not exist.")
            message_queue.put(("error", "Error", f"Input directory '{input_dir}' does not exist."))
            return

        os.makedirs(output_dir, exist_ok=True)
        logger.debug(f"Output directory '{output_dir}' is ready.")

        wav_files = [f for f in os.listdir(input_dir) if os.path.isfile(os.path.join(input_dir, f)) and f.lower().endswith(".wav")]
        if not wav_files:
            logger.error(f"No .wav files found in directory '{input_dir}'.")
            message_queue.put(("error", "Error", f"No .wav files found in directory '{input_dir}'."))
            return

        logger.info(f"Found {len(wav_files)} .wav file(s) to process.")
        processed_files = 0
        error_files = 0

        # Get naming scheme and custom names
        naming_scheme = naming_scheme_var.get()
        custom_names = custom_names_var.get().split(",") if naming_scheme == "custom" else []

        for idx, wav_file in enumerate(wav_files):
            input_file = os.path.join(input_dir, wav_file)
            logger.info(f"Processing file: {input_file}")

            progress = int(((idx + 1) / total_files) * 100)
            progress_var.set(progress)
            progress_bar["value"] = progress
            message_queue.put(("progress", "Progress", f"{progress}%"))
            progress_bar.update_idletasks()

            try:
                # Set ffprobe path before loading each file
                AudioSegment.ffprobe = ffprobe_path
                logger.debug(f"AudioSegment.ffprobe before loading '{wav_file}': {AudioSegment.ffprobe}")

                # Ensure ffmpeg directory is in PATH before loading each file
                if ffmpeg_dir not in os.environ["PATH"]:
                    os.environ["PATH"] += os.pathsep + ffmpeg_dir
                    logger.debug(f"Before loading '{wav_file}': Updated PATH with ffmpeg directory: {ffmpeg_dir}")

                audio = AudioSegment.from_file(input_file)
                logger.debug(f"Loaded audio file '{input_file}' successfully.")
            except Exception as e:
                logger.error(f"Error loading audio file '{input_file}': {e}")
                logger.debug(traceback.format_exc())
                message_queue.put(("error", "Error", f"Error loading audio file '{input_file}': {e}"))
                error_files += 1
                continue

            metadata = get_metadata(input_file, ffprobe_path)

            bits_per_sample = get_bits_per_sample(input_file, ffprobe_path)
            if bits_per_sample is None:
                message_queue.put(("error", "Error", f"Could not determine bit depth of '{wav_file}'"))
                error_files += 1
                continue

            original_frame_rate = audio.frame_rate
            original_channels = audio.channels
            logger.info(f"Original sample rate: {original_frame_rate} Hz")
            logger.info(f"Original bit depth: {bits_per_sample} bits")
            logger.info(f"Number of channels in '{wav_file}': {original_channels}")

            try:
                channels = audio.split_to_mono()
                logger.debug(f"Split audio into {len(channels)} mono channel(s).")
            except Exception as e:
                logger.error(f"Error splitting channels for '{wav_file}': {e}")
                logger.debug(traceback.format_exc())
                message_queue.put(("error", "Error", f"Error splitting channels for '{wav_file}': {e}"))
                error_files += 1
                continue

            # Adjust selected_channels based on the current file's channel count
            channels_to_process = [c for c in selected_channels if c <= original_channels]
            if not channels_to_process:
                logger.warning(f"No valid channels selected for '{wav_file}' based on available channels: {original_channels}")
                continue

            for channel_idx, channel in enumerate(channels):
                channel_number = channel_idx + 1

                if channel_number not in channels_to_process:
                    continue

                sample_fmt = get_sample_fmt(override_bit_depth or bits_per_sample)
                if sample_fmt is None:
                    logger.error(f"Unsupported bit depth: {override_bit_depth or bits_per_sample} bits in '{wav_file}'")
                    message_queue.put(("error", "Error", f"Unsupported bit depth: {override_bit_depth or bits_per_sample} bits in '{wav_file}'"))
                    error_files += 1
                    continue

                codec_mapping = {"u8": "pcm_u8", "s16": "pcm_s16le", "s24": "pcm_s24le", "s32": "pcm_s32le"}
                codec = codec_mapping.get(sample_fmt, "pcm_s16le")
                logger.debug(f"Using codec '{codec}' for sample_fmt '{sample_fmt}'.")

                base_name, _ = os.path.splitext(wav_file)
                if naming_scheme == "custom" and channel_idx < len(custom_names):
                    output_filename = f"{base_name}_{custom_names[channel_idx]}.wav"
                else:
                    output_filename = f"{base_name}_chan{channel_number}.wav"
                output_file = os.path.join(output_dir, output_filename)
                logger.debug(f"Output file will be '{output_file}'.")

                if override_sample_rate:
                    channel = channel.set_frame_rate(override_sample_rate)
                    logger.debug(f"Set frame rate to {override_sample_rate} Hz for channel {channel_number}.")
                else:
                    channel = channel.set_frame_rate(original_frame_rate)
                    logger.debug(f"Maintained original frame rate of {original_frame_rate} Hz for channel {channel_number}.")

                try:
                    channel.export(output_file, format="wav", parameters=["-c:a", codec])
                    logger.info(f"Exported: {output_file}")
                except Exception as e:
                    logger.error(f"Error exporting file '{output_file}': {e}")
                    logger.debug(traceback.format_exc())
                    message_queue.put(("error", "Error", f"Error exporting file '{output_file}': {e}"))
                    error_files += 1

            processed_files += 1

        progress_var.set(100)
        progress_bar["value"] = 100
        message_queue.put(("progress", "Progress", "100%"))
        progress_bar.update_idletasks()

        summary = f"Processed {processed_files} out of {len(wav_files)} files.\n"
        if error_files > 0:
            summary += f"Encountered errors in {error_files} file(s).\n"
        summary += f"Output Directory: {output_dir}"

        message_queue.put(("info", "Processing Complete", summary))
        logger.info("Audio splitting process completed.")

    except Exception as e:
        logger.error(f"An unexpected error occurred in split_audio_files: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"An unexpected error occurred:\n{e}"))

def browse_input_dir(message_queue):
    global last_input_dir
    try:
        # Use the current value of input_dir_var as the initial directory
        initial_dir = input_dir_var.get() if os.path.isdir(input_dir_var.get()) else last_input_dir
        directory = filedialog.askdirectory(initialdir=initial_dir)
        if directory:
            input_dir_var.set(directory)
            logger.debug(f"Selected input directory: {directory}")
            last_input_dir = directory
            update_file_count()
    except Exception as e:
        logger.error(f"Error selecting input directory: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error selecting input directory: {e}"))

def browse_output_dir(message_queue):
    global last_output_dir
    try:
        # Use the current value of output_dir_var as the initial directory
        initial_dir = output_dir_var.get() if os.path.isdir(output_dir_var.get()) else last_output_dir
        directory = filedialog.askdirectory(initialdir=initial_dir)
        if directory:
            output_dir_var.set(directory)
            logger.debug(f"Selected output directory: {directory}")
            last_output_dir = directory
    except Exception as e:
        logger.error(f"Error selecting output directory: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error selecting output directory: {e}"))

def update_file_count():
    input_dir = input_dir_var.get()
    if os.path.isdir(input_dir):
        wav_files = [f for f in os.listdir(input_dir) if f.lower().endswith('.wav')]
        file_count_var.set(f"Files to process: {len(wav_files)}")
    else:
        file_count_var.set("Files to process: 0")

def run_splitter(message_queue):
    logger.debug("run_splitter function called.")
    try:
        input_dir = input_dir_var.get()
        output_dir = output_dir_var.get()
        logger.debug(f"Input Directory: {input_dir}")
        logger.debug(f"Output Directory: {output_dir}")
        if not input_dir or not output_dir:
            logger.error("Input or output directory not selected.")
            message_queue.put(("error", "Error", "Please select both input and output directories."))
            return

        split_button.config(state="disabled")

        wav_files = [f for f in os.listdir(input_dir) if os.path.isfile(os.path.join(input_dir, f)) and f.lower().endswith(".wav")]
        total_files = len(wav_files) if wav_files else 1

        override_sample_rate = int(sample_rate_var.get()) if override_sample_rate_var.get() else None
        override_bit_depth = int(bit_depth_var.get()) if override_bit_depth_var.get() else None
        selected_channels = [i+1 for i, var in enumerate(channel_vars) if var.get()]

        if not selected_channels:
            logger.warning("No channels selected. All channels will be processed by default.")
            selected_channels = list(range(1, 9))  # Assuming max 8 channels

        # Start the splitting process in a separate thread
        threading.Thread(
            target=split_audio_files,
            args=(
                input_dir,
                output_dir,
                progress_var,
                progress_bar,
                total_files,
                message_queue,
                ffprobe_path,
                override_sample_rate,
                override_bit_depth,
                selected_channels
            ),
            daemon=True,
        ).start()
    except Exception as e:
        logger.error(f"Error in run_splitter: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"An unexpected error occurred:\n{e}"))

def open_output_directory(output_dir):
    try:
        if os.name == "nt":  # For Windows
            os.startfile(output_dir)
        elif sys.platform == "darwin":  # For macOS
            subprocess.Popen(["open", output_dir])
        else:  # For Linux and other OS
            subprocess.Popen(["xdg-open", output_dir])
        logger.debug(f"Opened output directory: {output_dir}")
    except Exception as e:
        logger.error(f"Failed to open output directory '{output_dir}': {e}")
        logger.debug(traceback.format_exc())
        messagebox.showerror("Error", f"Failed to open output directory:\n{e}")

CONFIG_FILE = os.path.join(get_application_root(), "config.json")

def load_config():
    global last_input_dir, last_output_dir
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r") as f:
                config = json.load(f)
                last_input_dir = config.get("last_input_dir", os.path.expanduser("~"))
                last_output_dir = config.get("last_output_dir", os.path.expanduser("~"))
                logger.debug(f"Loaded config: {config}")
        except Exception as e:
            logger.error(f"Error loading config: {e}")
            logger.debug(traceback.format_exc())

def save_config():
    config = {"last_input_dir": last_input_dir, "last_output_dir": last_output_dir}
    try:
        with open(CONFIG_FILE, "w") as f:
            json.dump(config, f)
            logger.debug(f"Saved config: {config}")
    except Exception as e:
        logger.error(f"Error saving config: {e}")
        logger.debug(traceback.format_exc())

def on_closing(root, message_queue):
    save_config()
    logger.info("Configuration saved. Exiting application.")
    root.destroy()

def main():
    try:
        load_config()

        global last_input_dir, last_output_dir
        global naming_scheme_var, custom_names_var, input_dir_var, file_count_var, output_dir_var
        global override_sample_rate_var, sample_rate_var, override_bit_depth_var, bit_depth_var, channel_vars
        global progress_var, progress_bar, split_button, open_output_button
        global sample_rate_dropdown, bit_depth_dropdown

        root = TkinterDnD.Tk()
        root.title("ZQ SFX Audio Splitter")
        root.geometry("800x700")  # Set a default size
        root.minsize(800, 700)  # Set a minimum size
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)

        main_frame = Frame(root)
        main_frame.grid(sticky="nsew")
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(0, weight=1)

        content_frame = Frame(main_frame)
        content_frame.grid(sticky="nsew", padx=5, pady=5)
        content_frame.columnconfigure(0, weight=1)

        naming_scheme_var = StringVar(value="default")
        custom_names_var = StringVar(value="L,R,C,lfe,Ls,Rs,Lss,Rss")
        input_dir_var = StringVar(value=last_input_dir)
        file_count_var = StringVar()
        file_count_var.set("Files to process: 0")
        output_dir_var = StringVar(value=last_output_dir)
        override_sample_rate_var = BooleanVar()
        sample_rate_var = StringVar(value="48000 Hz")
        override_bit_depth_var = BooleanVar()
        bit_depth_var = StringVar(value="16 bit")
        channel_vars = [BooleanVar(value=True) for _ in range(8)]
        progress_var = IntVar()

        message_queue = queue.Queue()

        root.protocol("WM_DELETE_WINDOW", lambda: on_closing(root, message_queue))

        # Set the font to Lato and increase the size for accessibility
        font_family = "Lato"
        font_size = 14

        # Single File Split Section
        single_file_frame = LabelFrame(content_frame, text="Single File Split", font=(font_family, font_size, "bold"))
        single_file_frame.grid(row=0, column=0, sticky="ew", padx=5, pady=5)
        single_file_frame.columnconfigure(0, weight=1)

        # Placeholder for Single File Split section
        Label(single_file_frame, text="Placeholder for Single File Split section", font=(font_family, font_size)).grid(row=0, column=0, sticky="w", padx=5, pady=5)

        # Batch File Split Section
        input_section_frame = LabelFrame(content_frame, text="Batch File Split", font=(font_family, font_size, "bold"))
        input_section_frame.grid(row=1, column=0, sticky="ew", padx=5, pady=5)
        input_section_frame.columnconfigure(0, weight=1)

        Label(input_section_frame, text="Input Directory:", width=15, anchor='w', font=(font_family, font_size)).grid(row=0, column=0, sticky="w", padx=5, pady=5)
        input_dir_entry = Entry(input_section_frame, textvariable=input_dir_var, font=(font_family, font_size))
        input_dir_entry.grid(row=0, column=1, sticky="w", padx=(0, 5), pady=5)  # Adjust padding to remove extra space
        input_dir_entry.drop_target_register(DND_FILES)
        input_dir_entry.dnd_bind('<<Drop>>', lambda event: handle_drop(event, input_dir_var, message_queue))
        Button(input_section_frame, text="Browse...", command=lambda: browse_input_dir(message_queue), font=(font_family, font_size)).grid(row=0, column=2, sticky="w", padx=5, pady=5)
        Label(input_section_frame, textvariable=file_count_var, font=(font_family, font_size, "italic")).grid(row=1, column=1, sticky="w", padx=5, pady=5)

        # Split File Location Section
        output_section_frame = LabelFrame(content_frame, text="Split File Location", font=(font_family, font_size, "bold"))
        output_section_frame.grid(row=2, column=0, sticky="ew", padx=5, pady=5)
        output_section_frame.columnconfigure(0, weight=1)

        Label(output_section_frame, text="Output Directory:", width=15, anchor='w', font=(font_family, font_size)).grid(row=0, column=0, sticky="w", padx=5, pady=5)
        output_dir_entry = Entry(output_section_frame, textvariable=output_dir_var, font=(font_family, font_size))
        output_dir_entry.grid(row=0, column=1, sticky="w", padx=(0, 5), pady=5)  # Adjust padding to remove extra space
        output_dir_entry.drop_target_register(DND_FILES)
        output_dir_entry.dnd_bind('<<Drop>>', lambda event: handle_drop(event, output_dir_var, message_queue))
        Button(output_section_frame, text="Browse...", command=lambda: browse_output_dir(message_queue), font=(font_family, font_size)).grid(row=0, column=2, sticky="w", padx=5, pady=5)

        # Override Options Section
        override_frame = LabelFrame(content_frame, text="Override Options", font=(font_family, font_size, "bold"))
        override_frame.grid(row=3, column=0, sticky="ew", padx=5, pady=5)
        override_frame.columnconfigure(0, weight=1)

        Checkbutton(
            override_frame,
            text="Override Sample Rate",
            variable=override_sample_rate_var,
            command=toggle_sample_rate_dropdown,
            font=(font_family, font_size)
        ).grid(row=0, column=0, sticky="w", padx=5, pady=5)

        sample_rate_dropdown = ttk.Combobox(
            override_frame,
            textvariable=sample_rate_var,
            values=["11025 Hz", "22050 Hz", "44100 Hz", "48000 Hz", "96000 Hz"],
            state="disabled",
            width=15,
            font=(font_family, font_size)
        )
        sample_rate_dropdown.grid(row=0, column=1, sticky="w", padx=5, pady=5)

        Checkbutton(
            override_frame,
            text="Override Bit Depth",
            variable=override_bit_depth_var,
            command=toggle_bit_depth_dropdown,
            font=(font_family, font_size)
        ).grid(row=1, column=0, sticky="w", padx=5, pady=5)

        bit_depth_dropdown = ttk.Combobox(
            override_frame,
            textvariable=bit_depth_var,
            values=["8 bit", "16 bit", "24 bit", "32 bit"],
            state="disabled",
            width=15,
            font=(font_family, font_size)
        )
        bit_depth_dropdown.grid(row=1, column=1, sticky="w", padx=5, pady=5)

        # Ensure the columns do not expand
        for col in range(2):
            override_frame.columnconfigure(col, weight=0)

        # Channel Selection and Naming Scheme Section
        channel_naming_frame = Frame(content_frame)
        channel_naming_frame.grid(row=4, column=0, sticky="ew", padx=5, pady=5)
        channel_naming_frame.columnconfigure(0, weight=1)
        channel_naming_frame.columnconfigure(1, weight=1)

        channel_frame = LabelFrame(channel_naming_frame, text="Channel Selection", font=(font_family, font_size, "bold"))
        channel_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        channel_frame.columnconfigure(0, weight=1)

        Label(channel_frame, text="Select Channels to Process:", font=(font_family, font_size)).grid(row=0, column=0, columnspan=4, sticky="w", padx=5, pady=5)

        # Lock the positions of the channel buttons
        for idx in range(8):
            Checkbutton(
                channel_frame,
                text=f"Channel {idx + 1}",
                variable=channel_vars[idx],
                font=(font_family, font_size)
            ).grid(row=1 + idx // 4, column=idx % 4, sticky="w", padx=5, pady=2)

        # Ensure the columns do not expand
        for col in range(4):
            channel_frame.columnconfigure(col, weight=0)

        naming_scheme_frame = LabelFrame(channel_naming_frame, text="Channel Naming", font=(font_family, font_size, "bold"))
        naming_scheme_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        naming_scheme_frame.columnconfigure(0, weight=1)

        Label(naming_scheme_frame, text="Naming Scheme:", font=(font_family, font_size)).grid(row=0, column=0, columnspan=2, sticky="w", padx=5, pady=5)

        default_naming_radio = Radiobutton(
            naming_scheme_frame,
            text="Default (chan1, chan2, ...)",
            variable=naming_scheme_var,
            value="default",
            font=(font_family, font_size)
        )
        default_naming_radio.grid(row=1, column=0, sticky="w", padx=5, pady=5)

        custom_naming_radio = Radiobutton(
            naming_scheme_frame,
            text="Custom",
            variable=naming_scheme_var,
            value="custom",
            font=(font_family, font_size)
        )
        custom_naming_radio.grid(row=2, column=0, sticky="w", padx=5, pady=5)

        custom_names_entry = Entry(naming_scheme_frame, textvariable=custom_names_var, state="disabled", font=(font_family, font_size))
        custom_names_entry.grid(row=3, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

        def toggle_custom_names_entry():
            if naming_scheme_var.get() == "custom":
                custom_names_entry.config(state="normal")
            else:
                custom_names_entry.config(state="disabled")

        naming_scheme_var.trace("w", lambda *args: toggle_custom_names_entry())

        # Progress Bar and Buttons Section
        progress_frame = Frame(content_frame)
        progress_frame.grid(row=5, column=0, sticky="ew", padx=5, pady=5)
        progress_frame.columnconfigure(0, weight=1)

        progress_bar = ttk.Progressbar(
            progress_frame,
            orient="horizontal",
            mode="determinate",
            variable=progress_var,
        )
        progress_bar.grid(row=0, column=0, sticky="ew", padx=5, pady=5)

        progress_label = Label(progress_frame, text="0%", font=(font_family, font_size))
        progress_label.grid(row=0, column=1, sticky="w", padx=5, pady=5)

        button_frame = Frame(content_frame)
        button_frame.grid(row=6, column=0, sticky="ew", padx=5, pady=5)
        button_frame.columnconfigure(0, weight=1)
        button_frame.columnconfigure(1, weight=1)

        split_button = Button(
            button_frame, text="Split Audio Files", command=lambda: run_splitter(message_queue), font=(font_family, font_size)
        )
        split_button.grid(row=0, column=0, pady=5, padx=5)

        open_output_button = Button(
            button_frame,
            text="Open Output Directory",
            command=lambda: open_output_directory(output_dir_var.get()),
            state="disabled",
            font=(font_family, font_size)
        )
        open_output_button.grid(row=0, column=1, pady=5, padx=5)

        def process_queue():
            try:
                while True:
                    msg_type, title, message = message_queue.get_nowait()
                    if msg_type == "info":
                        messagebox.showinfo(title, message)
                        open_output_button.config(state="normal")
                        split_button.config(state="normal")
                    elif msg_type == "error":
                        messagebox.showerror(title, message)
                        split_button.config(state="normal")
                    elif msg_type == "progress":
                        progress_label.config(text=message)
            except queue.Empty:
                pass
            root.after(100, process_queue)

        root.after(100, process_queue)

        # Initialize file count
        update_file_count()

        logger.info("ZQ SFX Audio Splitter application started.")
        logger.debug("Starting the Tkinter main loop.")
        root.mainloop()

    except Exception as e:
        logger.error("An unexpected error occurred in main:")
        logger.error(traceback.format_exc())
        messagebox.showerror("Error", f"An unexpected error occurred:\n{e}")
        sys.exit(1)

if __name__ == "__main__":
    main()