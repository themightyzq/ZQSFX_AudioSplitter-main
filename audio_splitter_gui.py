#!/usr/bin/env python3

import os
import sys
import logging
import traceback
import threading
import queue
import struct
import xml.etree.ElementTree as ET
from datetime import datetime
import tempfile
import shutil
from tkinterdnd2 import TkinterDnD, DND_FILES
from tkinter import (
    Label,
    Entry,
    StringVar,
    IntVar,
    BooleanVar,
    filedialog,
    messagebox,
    Frame,
    Checkbutton,
    Radiobutton,
    LabelFrame,
    Toplevel,
)
from tkinter import ttk
from pydub import AudioSegment
from pydub.utils import which
import subprocess
import json

# Define channel_checkboxes globally
channel_checkboxes = []
# Declare notebook as a global variable
notebook = None

def resource_path(relative_path):
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)
# Initialize tkdnd
if getattr(sys, 'frozen', False):
    tkdnd_path = resource_path('tkdnd')
    os.environ['TKDND_LIBRARY'] = tkdnd_path
else:
    tkdnd_path = os.path.join(os.path.dirname(__file__), 'tkdnd')
    os.environ['TKDND_LIBRARY'] = tkdnd_path

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
    if getattr(sys, "frozen", False):
        app_root = sys._MEIPASS
    else:
        app_root = os.path.dirname(os.path.abspath(__file__))
    return app_root

tkdnd_path = os.path.join(get_application_root(), "tkdnd")
if tkdnd_path not in sys.path:
    sys.path.append(tkdnd_path)

def get_log_file_path():
    home = os.path.expanduser("~")
    if sys.platform == "darwin":
        log_dir = os.path.join(home, "Library", "Logs", "ZQSFXAudioSplitter")
    elif sys.platform == "win32":
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

def update_button_states():
    current_tab = notebook.tab(notebook.select(), "text")
    single_file_path = single_file_var.get()
    output_dir = output_dir_var.get()
    input_dir = input_dir_var.get()

    logger.debug(f"Active Tab: {current_tab}")
    logger.debug(f"Single File Path: {single_file_path}")
    logger.debug(f"Input Directory: {input_dir}")
    logger.debug(f"Output Directory: {output_dir}")

    if current_tab == "Split Single File":
        if os.path.isfile(single_file_path) and os.path.isdir(output_dir):
            split_button.config(state="normal")
            open_output_button.config(state="normal")
            open_input_file_button.config(state="normal")
            logger.debug("Split and Open buttons enabled for Single File Split.")
        else:
            split_button.config(state="disabled")
            open_output_button.config(state="disabled")
            open_input_file_button.config(state="disabled")
            logger.debug("Split and Open buttons disabled for Single File Split.")
    elif current_tab == "Batch Split":
        if os.path.isdir(input_dir) and os.path.isdir(output_dir):
            split_button.config(state="normal")
            open_input_directory_button.config(state="normal")
            open_output_directory_button.config(state="normal")
            logger.debug("Split and Open buttons enabled for Batch Split.")
        else:
            split_button.config(state="disabled")
            open_input_directory_button.config(state="disabled")
            open_output_directory_button.config(state="disabled")
            logger.debug("Split and Open buttons disabled for Batch Split.")

def handle_drop(event, dir_var, message_queue):
    try:
        dropped_path = event.data.strip("{}")
        logger.debug(f"Dropped path: {dropped_path}")

        if os.path.isdir(dropped_path):
            dir_var.set(dropped_path)
            logger.debug(f"Directory set via drag-and-drop: {dropped_path}")
            update_file_count()
            update_button_states()
        elif os.path.isfile(dropped_path):
            if dir_var == single_file_var:
                dir_var.set(dropped_path)
                logger.debug(f"File set via drag-and-drop: {dropped_path}")
                update_channel_checkboxes()
                update_button_states()
            else:
                logger.error(f"Dropped item is not a directory: {dropped_path}")
                message_queue.put(("error", "Error", "Dropped item is not a directory."))
        else:
            logger.error(f"Dropped item is neither file nor directory: {dropped_path}")
            message_queue.put(("error", "Error", "Dropped item is neither a file nor a directory."))
    except Exception as e:
        logger.error(f"Error handling drag-and-drop: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error handling drag-and-drop: {e}"))

setup_logging()
logger = logging.getLogger(__name__)

last_input_dir = os.path.expanduser("~")
last_output_dir = os.path.expanduser("~")
last_dir = os.path.expanduser("~")

def get_ffmpeg_paths():
    app_root = get_application_root()

    if os.name == "nt":
        ffmpeg_filename = "ffmpeg.exe"
        ffprobe_filename = "ffprobe.exe"
    else:
        ffmpeg_filename = "ffmpeg"
        ffprobe_filename = "ffprobe"

    ffmpeg_path = os.path.join(app_root, "ffmpeg", ffmpeg_filename)
    ffprobe_path = os.path.join(app_root, "ffmpeg", ffprobe_filename)

    if not os.path.exists(ffmpeg_path):
        logger.debug(f"FFmpeg not found in '{ffmpeg_path}'. Searching in system PATH.")
        ffmpeg_path = which("ffmpeg")
    if not os.path.exists(ffprobe_path):
        logger.debug(
            f"FFprobe not found in '{ffprobe_path}'. Searching in system PATH."
        )
        ffprobe_path = which("ffprobe")

    logger.debug(f"FFmpeg Path: {ffmpeg_path}")
    logger.debug(f"FFprobe Path: {ffprobe_path}")

    if (
        ffmpeg_path
        and ffprobe_path
        and os.path.exists(ffmpeg_path)
        and os.path.exists(ffprobe_path)
    ):
        logger.info(f"Using FFmpeg at: {ffmpeg_path}")
        logger.info(f"Using FFprobe at: {ffprobe_path}")
        return ffmpeg_path, ffprobe_path
    else:
        logger.critical("FFmpeg and/or FFprobe not found. The application will exit.")
        messagebox.showerror(
            "Error",
            "FFmpeg and/or FFprobe not found. Please ensure they are installed and included with the application.",
        )
        sys.exit(1)

def get_bits_per_sample(file_path, ffprobe_path):
    try:
        cmd = [
            ffprobe_path,
            "-v",
            "error",
            "-select_streams",
            "a:0",
            "-show_entries",
            "stream=bits_per_sample",
            "-of",
            "default=noprint_wrappers=1:nokey=1",
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
        logger.debug(
            f"Mapped bits_per_sample {bits_per_sample} to sample_fmt {sample_fmt}"
        )
    return sample_fmt


ffmpeg_path, ffprobe_path = get_ffmpeg_paths()

AudioSegment.converter = ffmpeg_path
AudioSegment.ffprobe = ffprobe_path
logger.debug(f"AudioSegment.ffprobe set to: {AudioSegment.ffprobe}")

ffmpeg_dir = os.path.dirname(ffmpeg_path)
if ffmpeg_dir not in os.environ["PATH"]:
    os.environ["PATH"] += os.pathsep + ffmpeg_dir
    logger.debug(
        f"Updated PATH environment variable with ffmpeg directory: {ffmpeg_dir}"
    )

def split_audio_files(
    input_dir,
    output_dir,
    progress_var,
    progress_bar,
    total_files,
    message_queue,
    ffprobe_path,
    override_sample_rate,
    override_bit_depth,
    naming_scheme,
    custom_names,
):
    try:
        if not os.path.isdir(input_dir):
            logger.error(f"Input directory '{input_dir}' does not exist.")
            message_queue.put(
                ("error", "Error", f"Input directory '{input_dir}' does not exist.")
            )
            return

        os.makedirs(output_dir, exist_ok=True)
        logger.debug(f"Output directory '{output_dir}' is ready.")

        wav_files = [
            f
            for f in os.listdir(input_dir)
            if os.path.isfile(os.path.join(input_dir, f)) and f.lower().endswith(".wav")
        ]
        if not wav_files:
            logger.error(f"No .wav files found in directory '{input_dir}'.")
            message_queue.put(
                ("error", "Error", f"No .wav files found in directory '{input_dir}'.")
            )
            return

        logger.info(f"Found {len(wav_files)} .wav file(s) to process.")
        processed_files = 0
        error_files = 0

        for idx, wav_file in enumerate(wav_files):
            input_file = os.path.join(input_dir, wav_file)
            logger.info(f"Processing file: {input_file}")

            progress = int(((idx + 1) / total_files) * 100)
            progress_var.set(progress)
            progress_bar["value"] = progress
            message_queue.put(("progress", "Progress", f"{progress}%"))
            progress_bar.update_idletasks()

            try:
                # Get channel count using ffprobe
                cmd = [
                    ffprobe_path,
                    "-v", "error",
                    "-select_streams", "a:0",
                    "-show_entries", "stream=channels",
                    "-of", "default=noprint_wrappers=1:nokey=1",
                    input_file
                ]
                output = subprocess.check_output(cmd).decode().strip()
                total_channels = int(output)
                logger.debug(f"Total channels in '{wav_file}': {total_channels}")
            except Exception as e:
                logger.error(f"Error determining total channels for '{wav_file}': {e}")
                message_queue.put(
                    ("error", "Error", f"Error determining total channels for '{wav_file}': {e}")
                )
                error_files += 1
                continue

            # Process each channel
            for channel_idx in range(total_channels):
                try:
                    base_name, _ = os.path.splitext(wav_file)
                    if naming_scheme == "custom" and channel_idx < len(custom_names):
                        output_filename = f"{base_name}_{custom_names[channel_idx].strip()}.wav"
                    else:
                        output_filename = f"{base_name}_chan{channel_idx + 1}.wav"
                    output_file = os.path.join(output_dir, output_filename)
                    
                    success = run_ffmpeg_with_metadata(
                        input_file,
                        channel_idx,
                        output_file,
                        override_bit_depth,
                        override_sample_rate
                    )
                    
                    if success:
                        logger.info(f"Exported with metadata: {output_file}")
                    else:
                        raise Exception("Failed to export with metadata")
                        
                except Exception as e:
                    logger.error(f"Error processing channel {channel_idx + 1} of '{wav_file}': {e}")
                    logger.debug(traceback.format_exc())
                    message_queue.put(
                        ("error", "Error", f"Error processing channel {channel_idx + 1} of '{wav_file}': {e}")
                    )
                    error_files += 1
                    continue

            processed_files += 1

        # Remove debug_metadata folder after processing all files
        debug_metadata_path = os.path.join(output_dir, "debug_metadata")
        if os.path.exists(debug_metadata_path):
            try:
                shutil.rmtree(debug_metadata_path)
                logger.debug(f"Removed debug_metadata folder at {debug_metadata_path}")
            except Exception as e:
                logger.error(f"Failed to remove debug_metadata folder: {e}")
                message_queue.put(("error", "Error", f"Failed to remove debug_metadata folder: {e}"))

        progress_var.set(100)
        progress_bar["value"] = 100
        message_queue.put(("progress", None, "100%"))
        progress_bar.update_idletasks()

        # Replace detailed summary with "SUCCESS!"
        message_queue.put(("info", "Processing Complete", "SUCCESS!"))
        logger.info("Audio splitting process completed.")

    except Exception as e:
        logger.error(f"An unexpected error occurred in split_audio_files: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"An unexpected error occurred:\n{e}"))
    finally:
        message_queue.put(("enable_buttons", None, None))

def browse_input_dir(message_queue):
    global last_dir
    try:
        initial_dir = last_dir if os.path.isdir(last_dir) else os.path.expanduser("~")
        directory = filedialog.askdirectory(initialdir=initial_dir)
        if directory:
            input_dir_var.set(directory)
            logger.debug(f"Selected input directory: {directory}")
            last_dir = directory
            update_file_count()
            update_button_states()
    except Exception as e:
        logger.error(f"Error selecting input directory: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error selecting input directory: {e}"))

def browse_output_dir(message_queue):
    global last_dir
    try:
        initial_dir = last_dir if os.path.isdir(last_dir) else os.path.expanduser("~")
        directory = filedialog.askdirectory(initialdir=initial_dir)
        if directory:
            output_dir_var.set(directory)
            logger.debug(f"Selected output directory: {directory}")
            last_dir = directory
            update_button_states()
    except Exception as e:
        logger.error(f"Error selecting output directory: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"Error selecting output directory: {e}"))

def update_file_count():
    input_dir = input_dir_var.get()
    if os.path.isdir(input_dir):
        wav_files = [f for f in os.listdir(input_dir) if f.lower().endswith(".wav")]
        file_count_var.set(f"Files to process: {len(wav_files)}")
    else:
        file_count_var.set("Files to process: 0")

def run_splitter(message_queue):
    split_button.config(state="disabled")
    # Removed disabling of Open buttons to keep them enabled during splitting
    logger.debug("run_splitter function called.")
    try:
        input_dir = input_dir_var.get()
        output_dir = output_dir_var.get()
        logger.debug(f"Input Directory: {input_dir}")
        logger.debug(f"Output Directory: {output_dir}")
        if (
            not input_dir
            or input_dir == "Please select an input directory"
            or not os.path.isdir(input_dir)
        ):
            logger.error("Input directory not selected or invalid.")
            message_queue.put(
                (
                    "error",
                    "Error",
                    "Input directory is required. Please select a valid directory before proceeding.",
                )
            )
            return
        if (
            not output_dir
            or output_dir == "Please select an output directory"
            or not os.path.isdir(output_dir)
        ):
            logger.error("Output directory not selected or invalid.")
            message_queue.put(
                (
                    "error",
                    "Error",
                    "Output directory is required. Please select a valid directory before proceeding.",
                )
            )
            return

        split_button.config(state="disabled")

        wav_files = [
            f
            for f in os.listdir(input_dir)
            if os.path.isfile(os.path.join(input_dir, f)) and f.lower().endswith(".wav")
        ]
        total_files = len(wav_files) if wav_files else 1

        override_sample_rate = (
            int(sample_rate_var.get().split()[0])
            if override_sample_rate_var.get()
            else None
        )
        override_bit_depth = (
            int(bit_depth_var.get().split()[0])
            if override_bit_depth_var.get()
            else None
        )
        selected_channels = [i + 1 for i, var in enumerate(channel_vars) if var.get()]

        if not selected_channels:
            logger.warning(
                "No channels selected. All channels will be processed by default."
            )
            selected_channels = list(range(1, 9))

        naming_scheme = naming_scheme_var.get()
        custom_names = (
            custom_names_var.get().split(",") if naming_scheme == "custom" else []
        )

        # Trim whitespace from custom names
        custom_names = [name.strip() for name in custom_names]

        # Optional: Validate that there are enough custom names
        if naming_scheme == "custom" and len(custom_names) < 8:
            logger.warning(
                "Not enough custom names provided. Some channels will use default naming."
            )

        # Start the thread with the current naming_scheme and custom_names
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
                naming_scheme,
                custom_names,
            ),
            daemon=True,
        ).start()
    except Exception as e:
        logger.error(f"Error in run_splitter: {e}")
        logger.debug(traceback.format_exc())
        message_queue.put(("error", "Error", f"An unexpected error occurred:\n{e}"))

def open_output_directory(output_dir):
    try:
        if os.name == "nt":
            os.startfile(output_dir)
        elif sys.platform == "darwin":
            subprocess.Popen(["open", output_dir])
        else:
            subprocess.Popen(["xdg-open", output_dir])
        logger.debug(f"Opened output directory: {output_dir}")
    except Exception as e:
        logger.error(f"Failed to open output directory '{output_dir}': {e}")
        logger.debug(traceback.format_exc())
        messagebox.showerror("Error", f"Failed to open output directory:\n{e}")

def open_file_directory(file_path):
    directory = os.path.dirname(file_path)
    if os.path.isdir(directory):
        if os.name == "nt":
            os.startfile(directory)
        elif sys.platform == "darwin":
            subprocess.Popen(["open", directory])
        else:
            subprocess.Popen(["xdg-open", directory])
        logger.debug(f"Opened directory containing file: {directory}")
    else:
        logger.error(f"Directory does not exist: {directory}")
        messagebox.showerror("Error", f"Directory does not exist:\n{directory}")

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
    if not last_output_dir:
        last_output_dir = last_input_dir

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



def handle_single_file_drop(event, file_var, message_queue):
    try:
        dropped_path = event.data.strip("{}")
        if os.path.isfile(dropped_path):
            file_var.set(dropped_path)
            update_channel_checkboxes()
            update_button_states()
    except Exception as e:
        logger.error(f"Error handling drag-and-drop: {e}")
        message_queue.put(("error", "Error", f"Error handling drag-and-drop: {e}"))

def browse_single_file(message_queue):
    global last_dir
    try:
        initial_dir = last_dir if os.path.isdir(last_dir) else os.path.expanduser("~")
        file_path = filedialog.askopenfilename(
            initialdir=initial_dir, filetypes=[("Audio Files", "*.wav")]
        )
        if file_path:
            single_file_var.set(file_path)
            last_dir = os.path.dirname(file_path)
            update_channel_checkboxes()
            update_button_states()
    except Exception as e:
        logger.error(f"Error selecting file: {e}")
        message_queue.put(("error", "Error", f"Error selecting file: {e}"))

def split_single_file(message_queue):
    split_button.config(state="disabled")
    open_output_button.config(state="disabled")  # Temporarily disable if needed
    open_input_file_button.config(state="disabled")  # Temporarily disable if needed
    try:
        file_path = single_file_var.get()
        output_dir = output_dir_var.get()
        logger.debug(f"File Path: {file_path}")
        logger.debug(f"Output Directory: {output_dir}")

        if (
            not file_path
            or file_path == "Please select a file to split"
            or not os.path.isfile(file_path)
        ):
            logger.error("File directory not selected or invalid.")
            message_queue.put(
                (
                    "error",
                    "Error",
                    "File is required. Please select a valid file before proceeding.",
                )
            )
            return
        if (
            not output_dir
            or output_dir == "Please select an output directory"
            or not os.path.isdir(output_dir)
        ):
            logger.error("Output directory not selected or invalid.")
            message_queue.put(
                (
                    "error",
                    "Error",
                    "Output directory is required. Please select a valid directory before proceeding.",
                )
            )
            return

        os.makedirs(output_dir, exist_ok=True)

        cmd = [
            ffprobe_path,
            "-v",
            "error",
            "-select_streams",
            "a:0",
            "-show_entries",
            "stream=channels",
            "-of",
            "default=noprint_wrappers=1:nokey=1",
            file_path,
        ]
        try:
            output = subprocess.check_output(cmd).decode().strip()
            total_channels = int(output)
            logger.debug(f"Total channels in input file: {total_channels}")
        except Exception as e:
            logger.error(f"Error determining total channels for '{file_path}': {e}")
            message_queue.put(
                (
                    "error",
                    "Error",
                    f"Error determining total channels for '{file_path}': {e}",
                )
            )
            return

        selected_channels = [
            idx for idx, var in enumerate(channel_vars[:total_channels]) if var.get()
        ]
        if not selected_channels:
            message_queue.put(
                (
                    "error",
                    "Error",
                    "Please select at least one channel to process.",
                )
            )
            return

        if override_bit_depth_var.get():
            override_bit_depth = int(bit_depth_var.get().split()[0])
        else:
            override_bit_depth = None

        progress_var.set(0)
        progress_bar.config(maximum=100)

        naming_scheme = naming_scheme_var.get()
        custom_names = (
            custom_names_var.get().split(",") if naming_scheme == "custom" else []
        )
        custom_names = [name.strip() for name in custom_names]

        if naming_scheme == "custom" and len(custom_names) < total_channels:
            logger.warning(
                "Not enough custom names provided. Some channels will use default naming."
            )

        logger.debug(f"Override Bit Depth: {override_bit_depth}")
        logger.debug(f"Selected Channels: {selected_channels}")
        logger.debug(f"Naming Scheme: {naming_scheme}")
        logger.debug(f"Custom Names: {custom_names}")

        for idx in selected_channels:
            if naming_scheme == "custom" and idx < len(custom_names):
                output_filename = f"{os.path.splitext(os.path.basename(file_path))[0]}_{custom_names[idx]}.wav"
            else:
                output_filename = f"{os.path.splitext(os.path.basename(file_path))[0]}_chan{idx + 1}.wav"

            output_file = os.path.join(output_dir, output_filename)
            run_ffmpeg_with_metadata(
                file_path,
                idx,
                output_file,
                override_bit_depth,
                override_sample_rate=(
                    int(sample_rate_var.get().split()[0])
                    if override_sample_rate_var.get()
                    else None
                ),
            )
            progress = int(
                ((selected_channels.index(idx) + 1) / len(selected_channels)) * 100
            )
            progress_var.set(progress)
            message_queue.put(("progress", None, f"{progress}%"))

        debug_metadata_path = os.path.join(output_dir, "debug_metadata")
        if os.path.exists(debug_metadata_path):
            try:
                shutil.rmtree(debug_metadata_path)
                logger.debug(f"Removed debug_metadata folder at {debug_metadata_path}")
            except Exception as e:
                logger.error(f"Failed to remove debug_metadata folder: {e}")
                message_queue.put(("error", "Error", f"Failed to remove debug_metadata folder: {e}"))

        progress_var.set(100)
        progress_bar["value"] = 100
        message_queue.put(("progress", None, "100%"))

        message_queue.put(
            (
                "info",
                "Splitting Complete",
                "SUCCESS!",
            )
        )
    except Exception as e:
        logger.error(f"An unexpected error occurred: {e}")
        message_queue.put(("error", "Error", f"An unexpected error occurred:\n{e}"))
    finally:
        split_button.config(state="normal")
        open_output_button.config(state="normal")
        open_input_file_button.config(state="normal")

def run_ffmpeg_with_metadata(input_file, channel_idx, output_file, override_bit_depth=None, override_sample_rate=None):
    """
    Process a single channel and preserve all metadata from source to output file
    Uses WAVMetadataReader to read all BWF and iXML chunks
    """
    try:
        # Create debug directory for logging
        debug_dir = os.path.join(os.path.dirname(output_file), "debug_metadata")
        os.makedirs(debug_dir, exist_ok=True)
        
        # Read all metadata using the improved WAVMetadataReader
        reader = WAVMetadataReader(input_file)
        source_metadata = reader.metadata
        
        # Log source metadata
        with open(os.path.join(debug_dir, "source_metadata.txt"), "w") as f:
            f.write(json.dumps(source_metadata, indent=2))

        # Build FFmpeg command with comprehensive metadata preservation
        cmd = [
            ffmpeg_path,
            '-y',
            '-i', input_file,
            '-map_metadata', '0',  # Preserve global metadata
            '-write_bext', '1',    # Enable BWF/BEXT writing
            '-write_id3v2', '1',   # Preserve ID3 tags if present
            '-map', '0:a:0',       # Map first audio stream
            '-af', f'pan=mono|c0=c{channel_idx}',
        ]

        # Add sample rate if specified
        if override_sample_rate:
            cmd.extend(['-ar', str(override_sample_rate)])

        # Add bit depth if specified
        if override_bit_depth:
            codec_mapping = {8: "pcm_u8", 16: "pcm_s16le", 24: "pcm_s24le", 32: "pcm_s32le"}
            codec = codec_mapping.get(override_bit_depth, "pcm_s24le")
            cmd.extend(['-c:a', codec])
        else:
            cmd.extend(['-c:a', 'pcm_s24le'])

        # Add comprehensive metadata preservation flags
        bwf_flags = [
            '-metadata_header_padding', '8192',  # Increased padding for more metadata
            '-rf64', 'auto',
            '-fflags', '+bitexact',
            '-map_metadata:s:a', '0:s:a',  # Copy stream metadata
        ]
        cmd.extend(bwf_flags)

        # Map core BWF fields
        bwf_fields = {
            'Description': 'description',
            'Originator': 'originator',
            'Reference': 'originator_reference',
            'OriginationDate': 'origination_date',
            'OriginationTime': 'origination_time'
        }

        # Map additional metadata fields
        ixml_fields = {
            'Note': 'ixml_note',
            'Scene': 'scene',
            'Take': 'take',
            'Tape': 'tape',
            'Project': 'project',
            'Category': 'category',
            'Subcategory': 'subcategory'
        }

        # Add all BWF metadata
        for source_key, ffmpeg_key in bwf_fields.items():
            if source_key in source_metadata and source_metadata[source_key]:
                cmd.extend(['-metadata', f'{ffmpeg_key}={source_metadata[source_key]}'])
                # Also add as bext metadata for maximum compatibility
                cmd.extend(['-metadata:s:a:0', f'bext_{ffmpeg_key}={source_metadata[source_key]}'])

        # Add all iXML metadata
        for source_key, ffmpeg_key in ixml_fields.items():
            if source_key in source_metadata and source_metadata[source_key]:
                cmd.extend(['-metadata', f'{ffmpeg_key}={source_metadata[source_key]}'])
                # Add as both global and stream metadata
                cmd.extend(['-metadata:s:a:0', f'ixml_{ffmpeg_key}={source_metadata[source_key]}'])

        # Add output file
        cmd.append(output_file)

        # Log full command
        with open(os.path.join(debug_dir, "ffmpeg_command.txt"), "w") as f:
            f.write(" ".join(cmd))

        # Execute command
        process = subprocess.run(cmd, capture_output=True, text=True)
        if process.returncode != 0:
            logger.error(f"FFmpeg error: {process.stderr}")
            with open(os.path.join(debug_dir, "error.txt"), "w") as f:
                f.write(process.stderr)
            return False

        # Verify output metadata
        output_reader = WAVMetadataReader(output_file)
        output_metadata = output_reader.metadata

        # Log output metadata
        with open(os.path.join(debug_dir, "output_metadata.txt"), "w") as f:
            f.write(json.dumps(output_metadata, indent=2))

        # Compare metadata (excluding technical fields that will change)
        excluded_fields = {'Duration', 'Format', 'NumChannels', 'Version', 
                         'Number of Channels', 'Sample Width', 'Frame Rate', 
                         'Number of Frames'}
        
        preserved = all(
            key in output_metadata and output_metadata[key] == value
            for key, value in source_metadata.items()
            if value and key not in excluded_fields
        )

        if not preserved:
            logger.warning("Some metadata was not preserved")
            with open(os.path.join(debug_dir, "metadata_diff.txt"), "w") as f:
                f.write("=== Missing Metadata ===\n")
                for key, value in source_metadata.items():
                    if value and key not in output_metadata:
                        f.write(f"{key}: {value}\n")
                f.write("\n=== Changed Metadata ===\n")
                for key, value in source_metadata.items():
                    if value and key in output_metadata and output_metadata[key] != value:
                        f.write(f"{key}:\n  Source: {value}\n  Output: {output_metadata[key]}\n")

        return True

    except Exception as e:
        logger.error(f"Error: {str(e)}")
        traceback.print_exc()
        return False

def add_placeholder(entry, placeholder_text):
    def on_focus_in(event):
        if entry.get() == placeholder_text:
            entry.delete(0, "end")
            entry.config(fg=FOREGROUND_COLOR)

    def on_focus_out(event):
        if not entry.get():
            entry.insert(0, placeholder_text)
            entry.config(fg=PLACEHOLDER_COLOR)

    entry.insert(0, placeholder_text)
    entry.config(fg=PLACEHOLDER_COLOR)
    entry.bind("<FocusIn>", on_focus_in)
    entry.bind("<FocusOut>", on_focus_out)

class ToolTip:
    def __init__(self, widget, text, font_family="Segoe UI", font_size=12):
        self.widget = widget
        self.text = text
        self.tip_window = None
        self.font_family = font_family
        self.font_size = font_size
        widget.bind("<Enter>", self.show_tooltip)
        widget.bind("<Leave>", self.hide_tooltip)

    def show_tooltip(self, event):
        x = self.widget.winfo_rootx() + 20
        y = self.widget.winfo_rooty() + self.widget.winfo_height() + 1
        self.tip_window = tw = Toplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.wm_geometry(f"+{x}+{y}")
        label = Label(
            tw,
            text=self.text,
            background=BACKGROUND_COLOR,
            foreground=FOREGROUND_COLOR,
            relief="solid",
            borderwidth=1,
            font=(self.font_family, self.font_size),
        )
        label.pack()

    def hide_tooltip(self, event):
        if self.tip_window:
            self.tip_window.destroy()
            self.tip_window = None

def update_channel_checkboxes():
    try:
        file_path = single_file_var.get()
        logger.debug(f"update_channel_checkboxes called with file_path: {file_path}")
        if not file_path or not os.path.isfile(file_path):
            logger.debug("File path is invalid or does not exist.")
            return

        # Use pydub to get the number of channels
        audio = AudioSegment.from_file(file_path)
        total_channels = audio.channels
        logger.debug(f"Number of channels from audio file: {total_channels}")

        for channel_idx, chk in channel_checkboxes:
            if channel_idx < total_channels:
                chk.config(state="normal")
                channel_vars[channel_idx].set(True)
                logger.debug(f"Enabled checkbox for Channel {channel_idx + 1}")
            else:
                chk.config(state="disabled")
                channel_vars[channel_idx].set(False)
                logger.debug(f"Disabled checkbox for Channel {channel_idx + 1}")
    except Exception as e:
        logger.error(f"Error in update_channel_checkboxes: {e}")
        logger.debug(traceback.format_exc())

def set_minimum_window_size(root):
    root.update_idletasks()
    width = root.winfo_width()
    height = root.winfo_height()
    root.minsize(width, height)
    logger.debug(f"Set minimum window size to {width}x{height}")

FONT_FAMILY = "Segoe UI"
FONT_SIZE = 12

FOREGROUND_COLOR = "#FFFFFF"
BACKGROUND_COLOR = "#2C2C2C"
PLACEHOLDER_COLOR = "#A9A9A9"

# Define button widths
browse_button_width = 10
open_button_width = 5


class WAVMetadataReader:
    def __init__(self, filepath):
        self.filepath = filepath
        self.metadata = {}
        self.read_metadata()
    
    def read_metadata(self):
        try:
            with open(self.filepath, 'rb') as file:
                if file.read(4) != b'RIFF':
                    return
                    
                file.seek(4, 1)  # Skip file size
                
                if file.read(4) != b'WAVE':
                    return
                
                while True:
                    try:
                        chunk_id = file.read(4)
                        if not chunk_id:
                            break
                            
                        chunk_size = struct.unpack('<I', file.read(4))[0]
                        
                        if chunk_id == b'bext':
                            # Read BEXT chunk completely
                            self.read_bext_chunk(file)
                        elif chunk_id == b'iXML':
                            xml_data = file.read(chunk_size)
                            self.read_ixml_chunk(xml_data)
                        else:
                            file.seek(chunk_size, 1)
                            
                    except struct.error:
                        break
        except Exception as e:
            logger.error(f"Error reading metadata: {e}")

    def read_bext_chunk(self, file):
        try:
            self.metadata['Description'] = self.read_string(file.read(256))
            self.metadata['Originator'] = self.read_string(file.read(32))
            self.metadata['OriginatorReference'] = self.read_string(file.read(32))
            self.metadata['OriginationDate'] = self.read_string(file.read(10))
            self.metadata['OriginationTime'] = self.read_string(file.read(8))
            self.metadata['TimeReference'] = str(struct.unpack('<Q', file.read(8))[0])
            self.metadata['Version'] = str(struct.unpack('<H', file.read(2))[0])
            self.metadata['UMID'] = file.read(64).hex()
            
            # Read loudness metadata
            self.metadata['LoudnessValue'] = str(struct.unpack('<H', file.read(2))[0])
            self.metadata['LoudnessRange'] = str(struct.unpack('<H', file.read(2))[0])
            self.metadata['MaxTruePeakLevel'] = str(struct.unpack('<H', file.read(2))[0])
            self.metadata['MaxMomentaryLoudness'] = str(struct.unpack('<H', file.read(2))[0])
            self.metadata['MaxShortTermLoudness'] = str(struct.unpack('<H', file.read(2))[0])
            
            # Reserved bytes
            file.seek(180, 1)
            
            # Coding history (remaining bytes in chunk)
            self.metadata['CodingHistory'] = self.read_string(file.read())
            
        except Exception as e:
            logger.error(f"Error reading BEXT chunk: {e}")

    def read_ixml_chunk(self, xml_data):
        try:
            xml_str = xml_data.decode('utf-8', errors='ignore')
            root = ET.fromstring(xml_str)
            
            # Standard iXML fields
            xml_tags = {
                'NOTE': 'Note',
                'PROJECT': 'Project',
                'TAPE': 'Tape',
                'SCENE': 'Scene',
                'TAKE': 'Take',
                'FILE_UID': 'FileUID',
                'UBITS': 'UserBits',
                'CIRCLED': 'CircleTake'
            }
            
            for xml_tag, meta_key in xml_tags.items():
                element = root.find(f'.//{xml_tag}')
                if element is not None and element.text:
                    self.metadata[meta_key] = element.text.strip()
                    
            # Category metadata
            attr_list = root.findall('.//ATTR_LIST/ATTR')
            for attr in attr_list:
                name = attr.find('NAME')
                value = attr.find('VALUE')
                if name is not None and name.text == 'MusicalCategory' and value is not None:
                    parts = value.text.split('/')
                    if len(parts) >= 2:
                        self.metadata['Category'] = parts[0].strip()
                        self.metadata['Subcategory'] = parts[1].strip()
                    else:
                        self.metadata['Category'] = value.text.strip()
                        
        except Exception as e:
            logger.error(f"Error reading iXML chunk: {e}")

    def read_string(self, data):
        try:
            return data.split(b'\x00')[0].decode('utf-8').strip()
        except:
            return ''
        

def main():
    global split_button, open_output_directory_button, open_output_button, open_input_file_button, open_input_directory_button
    global notebook  # Declare notebook as global
    try:
        load_config()

        global last_input_dir, last_output_dir
        global naming_scheme_var, custom_names_var, input_dir_var, file_count_var, output_dir_var
        global override_sample_rate_var, sample_rate_var, override_bit_depth_var, bit_depth_var, channel_vars
        global progress_var, progress_bar
        global sample_rate_dropdown, bit_depth_dropdown
        global single_file_var, single_file_entry

        root = TkinterDnD.Tk()
        root.title("ZQ SFX Audio Splitter")
        root.configure(bg=BACKGROUND_COLOR)
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)

        font_family = "Segoe UI"
        font_size = 12

        notebook = ttk.Notebook(root)  # Assign to global notebook
        notebook.pack(fill="both", expand=True)
        

        single_file_tab = Frame(notebook, bg=BACKGROUND_COLOR)
        batch_tab = Frame(notebook, bg=BACKGROUND_COLOR)

        notebook.add(single_file_tab, text="Split Single File")
        notebook.add(batch_tab, text="Batch Split")

        naming_scheme_var = StringVar(value="default")
        custom_names_var = StringVar(value="L,R,C,lfe,Ls,Rs,Lss,Rss")
        input_dir_var = StringVar(value="")
        file_count_var = StringVar()
        file_count_var.set("Files to process: 0")
        output_dir_var = StringVar(value="")
        override_sample_rate_var = BooleanVar()
        sample_rate_var = StringVar(value="48000 Hz")  # Changed default to 48000 Hz
        override_bit_depth_var = BooleanVar()
        bit_depth_var = StringVar(value="16 bit")
        channel_vars = [BooleanVar(value=True) for _ in range(8)]
        progress_var = IntVar()
        single_file_var = StringVar()

        message_queue = queue.Queue()

        root.protocol("WM_DELETE_WINDOW", lambda: on_closing(root, message_queue))

        font_family = "Segoe UI"
        font_size = 12

        style = ttk.Style()
        style.theme_use("clam")

        style.configure(
            "Custom.TButton",
            background="#3C3C3C",
            foreground="#FFFFFF",
            font=(font_family, font_size),
            borderwidth=1,
            focuscolor=BACKGROUND_COLOR,
        )

        style.map(
            "Custom.TButton",
            background=[
                ("!active", "#3C3C3C"),
                ("pressed", "#2C2C2C"),
                ("active", "#5C5C5C"),
            ],
            foreground=[("disabled", "#7A7A7A"), ("!disabled", "#FFFFFF")],
        )

        style.configure(
            "Custom.Horizontal.TProgressbar",
            troughcolor="#3C3C3C",
            background="#00AA00",  # Green color
        )

        # **Custom style for Combobox**
        style.configure(
            "Custom.TCombobox",
            fieldbackground="#3C3C3C",
            background="#2C2C2C",
            foreground="#FFFFFF",
            selectbackground="#5C5C5C",
            selectforeground="#FFFFFF",
            arrowcolor="#FFFFFF",
            bordercolor="#2C2C2C",
            font=(font_family, font_size),
        )

        # **Add style map for different widget states**
        style.map(
            "Custom.TCombobox",
            fieldbackground=[("readonly", "#3C3C3C"), ("disabled", "#3C3C3C")],
            foreground=[("readonly", "#FFFFFF"), ("disabled", "#7A7A7A")],
            background=[("readonly", "#2C2C2C"), ("disabled", "#2C2C2C")],
            arrowcolor=[("readonly", "#FFFFFF"), ("disabled", "#7A7A7A")],
            bordercolor=[("readonly", "#2C2C2C"), ("disabled", "#2C2C2C")],
        )

        style.layout(
            "text.Horizontal.TProgressbar",
            [
                (
                    "Horizontal.Progressbar.trough",
                    {
                        "children": [
                            (
                                "Horizontal.Progressbar.pbar",
                                {"side": "left", "sticky": "ns"},
                            )
                        ],
                        "sticky": "nswe",
                    },
                ),
                ("Horizontal.Progressbar.label", {"sticky": ""}),
            ],
        )

        style.configure(
            "text.Horizontal.TProgressbar",
            text="0%",  # Initial text
            font=(font_family, font_size, "bold"),
            foreground=FOREGROUND_COLOR,
            background="#00AA00",  # Green color for the progress bar
            troughcolor="#3C3C3C",
        )

        # === Single File Tab ===
        single_file_tab.columnconfigure(0, weight=1)
        single_file_frame = LabelFrame(
            single_file_tab,
            text="Single File Split",
            font=(font_family, font_size, "bold"),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
        )
        single_file_frame.grid(row=0, column=0, sticky="ew", padx=5, pady=5)
        single_file_frame.columnconfigure(0, weight=0)
        single_file_frame.columnconfigure(1, weight=1)
        single_file_frame.columnconfigure(2, weight=0)
        single_file_frame.columnconfigure(3, weight=0)

        Label(
            single_file_frame,
            text="File to Split:",
            width=15,
            anchor="w",
            font=(FONT_FAMILY, FONT_SIZE),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=0, column=0, sticky="w", padx=5, pady=5)
        single_file_entry = Entry(
            single_file_frame,
            textvariable=single_file_var,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg="#3C3C3C",
        )
        single_file_entry.grid(row=0, column=1, sticky="ew", padx=(0, 5), pady=5)
        single_file_entry.drop_target_register(DND_FILES)
        single_file_entry.dnd_bind(
            "<<Drop>>",
            lambda event: handle_single_file_drop(
                event, single_file_var, message_queue
            ),
        )
        ttk.Button(
            single_file_frame,
            text="Browse...",
            command=lambda: browse_single_file(message_queue),
            style="Custom.TButton",
            width=browse_button_width,
        ).grid(row=0, column=2, sticky="w", padx=5, pady=5)

        open_input_file_button = ttk.Button(
            single_file_frame,
            text="Open",
            command=lambda: open_file_directory(single_file_var.get()),
            style="Custom.TButton",
            state="disabled",
            width=open_button_width,
        )
        open_input_file_button.grid(row=0, column=3, sticky="w", padx=5, pady=5)

        Label(
            single_file_frame,
            text="Output Directory:",
            width=15,
            anchor="w",
            font=(FONT_FAMILY, FONT_SIZE),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=1, column=0, sticky="w", padx=5, pady=5)

        output_dir_entry_single = Entry(
            single_file_frame,
            textvariable=output_dir_var,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg="#3C3C3C",
        )
        output_dir_entry_single.grid(row=1, column=1, sticky="ew", padx=(0, 5), pady=5)

        output_dir_entry_single.drop_target_register(DND_FILES)
        output_dir_entry_single.dnd_bind(
            "<<Drop>>", lambda event: handle_drop(event, output_dir_var, message_queue)
        )

        ttk.Button(
            single_file_frame,
            text="Browse...",
            command=lambda: browse_output_dir(message_queue),
            style="Custom.TButton",
            width=browse_button_width,
        ).grid(row=1, column=2, sticky="w", padx=5, pady=5)

        open_output_button = ttk.Button(
            single_file_frame,
            text="Open",
            command=lambda: open_output_directory(output_dir_var.get()),
            style="Custom.TButton",
            state="disabled",
            width=open_button_width,
        )
        open_output_button.grid(row=1, column=3, sticky="w", padx=5, pady=5)

        channel_frame = LabelFrame(
            single_file_frame,
            text="Channel Selection",
            font=(font_family, font_size, "bold"),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
        )
        channel_frame.grid(row=2, column=0, columnspan=4, sticky="ew", padx=5, pady=5)
        channel_frame.columnconfigure(0, weight=1)

        Label(
            channel_frame,
            text="Select Channels to Process:",
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=0, column=0, columnspan=4, sticky="w", padx=5, pady=5)

        # Initialize channel_checkboxes as a global list with desired layout
        for channel_idx in range(8):
            if channel_idx % 2 == 0:
                row = 1
                column = channel_idx // 2
            else:
                row = 2
                column = (channel_idx - 1) // 2
            chk = Checkbutton(
                channel_frame,
                text=f"Channel {channel_idx + 1}",
                variable=channel_vars[channel_idx],
                font=(font_family, font_size),
                fg=FOREGROUND_COLOR,
                bg=BACKGROUND_COLOR,
                selectcolor="#4A4A4A",  # Dark gray for selected state
                activeforeground=FOREGROUND_COLOR,
                activebackground=BACKGROUND_COLOR,
                highlightthickness=0,  # Remove focus highlight
            )
            chk.grid(row=row, column=column, sticky="w", padx=5, pady=2)
            channel_checkboxes.append((channel_idx, chk))

        for col in range(4):
            channel_frame.columnconfigure(col, weight=0)

        # === Batch Split Tab ===
        batch_tab.columnconfigure(0, weight=1)
        input_section_frame = LabelFrame(
            batch_tab,
            text="Batch File Split",
            font=(font_family, font_size, "bold"),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
        )
        input_section_frame.grid(row=0, column=0, sticky="ew", padx=5, pady=5)
        input_section_frame.columnconfigure(0, weight=0)
        input_section_frame.columnconfigure(1, weight=1)
        input_section_frame.columnconfigure(2, weight=0)
        input_section_frame.columnconfigure(3, weight=0)

        Label(
            input_section_frame,
            text="Input Directory:",
            width=15,
            anchor="w",
            font=(FONT_FAMILY, FONT_SIZE),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=0, column=0, sticky="w", padx=5, pady=5)
        input_dir_entry = Entry(
            input_section_frame,
            textvariable=input_dir_var,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg="#3C3C3C",
        )
        input_dir_entry.grid(row=0, column=1, sticky="ew", padx=(0, 5), pady=5)

        input_dir_entry.drop_target_register(DND_FILES)
        input_dir_entry.dnd_bind(
            "<<Drop>>", lambda event: handle_drop(event, input_dir_var, message_queue)
        )

        ttk.Button(
            input_section_frame,
            text="Browse...",
            command=lambda: browse_input_dir(message_queue),
            style="Custom.TButton",
            width=browse_button_width,
        ).grid(row=0, column=2, sticky="w", padx=5, pady=5)

        open_input_directory_button = ttk.Button(
            input_section_frame,
            text="Open",
            command=lambda: open_output_directory(input_dir_var.get()),
            style="Custom.TButton",
            state="disabled",
            width=open_button_width,
        )
        open_input_directory_button.grid(row=0, column=3, sticky="w", padx=5, pady=5)

        Label(
            input_section_frame,
            textvariable=file_count_var,
            font=(font_family, font_size, "italic"),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=1, column=1, sticky="w", padx=5, pady=5)

        Label(
            input_section_frame,
            text="Output Directory:",
            width=15,
            anchor="w",
            font=(FONT_FAMILY, FONT_SIZE),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=2, column=0, sticky="w", padx=5, pady=5)
        output_dir_entry_batch = Entry(
            input_section_frame,
            textvariable=output_dir_var,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg="#3C3C3C",
        )
        output_dir_entry_batch.grid(row=2, column=1, sticky="ew", padx=(0, 5), pady=5)

        output_dir_entry_batch.drop_target_register(DND_FILES)
        output_dir_entry_batch.dnd_bind(
            "<<Drop>>", lambda event: handle_drop(event, output_dir_var, message_queue)
        )

        ttk.Button(
            input_section_frame,
            text="Browse...",
            command=lambda: browse_output_dir(message_queue),
            style="Custom.TButton",
            width=browse_button_width,
        ).grid(row=2, column=2, sticky="w", padx=5, pady=5)

        open_output_directory_button = ttk.Button(
            input_section_frame,
            text="Open",
            command=lambda: open_output_directory(output_dir_var.get()),
            style="Custom.TButton",
            state="disabled",
            width=open_button_width,
        )
        open_output_directory_button.grid(row=2, column=3, sticky="w", padx=5, pady=5)

        # === Options Section ===
        options_frame = Frame(root, bg=BACKGROUND_COLOR)
        options_frame.pack(fill="x", padx=5, pady=5)
        options_frame.columnconfigure(0, weight=1)
        options_frame.columnconfigure(1, weight=1)

        override_quality_frame = LabelFrame(
            options_frame,
            text="Override Quality Settings",
            font=(font_family, font_size, "bold"),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
        )
        override_quality_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        override_quality_frame.columnconfigure(0, weight=1)
        override_quality_frame.columnconfigure(1, weight=1)

        Checkbutton(
            override_quality_frame,
            text="Override Sample Rate",
            variable=override_sample_rate_var,
            command=toggle_sample_rate_dropdown,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
            selectcolor="#4A4A4A",  # Dark gray for selected state
            activeforeground=FOREGROUND_COLOR,
            activebackground=BACKGROUND_COLOR,
            highlightthickness=0,  # Remove focus highlight
        ).grid(row=0, column=0, sticky="w", padx=5, pady=5)

        sample_rate_dropdown = ttk.Combobox(
            override_quality_frame,
            textvariable=sample_rate_var,
            values=["11025 Hz", "22050 Hz", "44100 Hz", "48000 Hz", "96000 Hz"],
            state="disabled",
            width=15,
            font=(font_family, font_size),
            style="Custom.TCombobox",  # Apply the custom style here
        )
        sample_rate_dropdown.grid(row=0, column=1, sticky="w", padx=5, pady=5)

        Checkbutton(
            override_quality_frame,
            text="Override Bit Depth",
            variable=override_bit_depth_var,
            command=toggle_bit_depth_dropdown,
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
            selectcolor="#4A4A4A",  # Dark gray for selected state
            activeforeground=FOREGROUND_COLOR,
            activebackground=BACKGROUND_COLOR,
            highlightthickness=0,  # Remove focus highlight
        ).grid(row=1, column=0, sticky="w", padx=5, pady=5)

        bit_depth_dropdown = ttk.Combobox(
            override_quality_frame,
            textvariable=bit_depth_var,
            values=["8 bit", "16 bit", "24 bit", "32 bit"],
            state="disabled",
            width=15,
            font=(font_family, font_size),
            style="Custom.TCombobox",  # Apply the custom style here
        )
        bit_depth_dropdown.grid(row=1, column=1, sticky="w", padx=5, pady=5)

        naming_scheme_frame = LabelFrame(
            options_frame,
            text="Channel Naming",
            font=(font_family, font_size, "bold"),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
        )
        naming_scheme_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        naming_scheme_frame.columnconfigure(0, weight=1)
        naming_scheme_frame.columnconfigure(1, weight=1)

        Label(
            naming_scheme_frame,
            text="Naming Scheme:",
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg=BACKGROUND_COLOR,
        ).grid(row=0, column=0, columnspan=2, sticky="w", padx=5, pady=5)

        default_naming_radio = Radiobutton(
            naming_scheme_frame,
            text="Default (chan1, chan2, ...)",
            variable=naming_scheme_var,
            value="default",
            font=(font_family, font_size),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
            selectcolor="#4A4A4A",  # Dark gray for selected state
            activeforeground=FOREGROUND_COLOR,
            activebackground=BACKGROUND_COLOR,
            highlightthickness=0,  # Remove focus highlight
        )
        default_naming_radio.grid(
            row=1, column=0, columnspan=2, sticky="w", padx=5, pady=5
        )

        custom_naming_radio = Radiobutton(
            naming_scheme_frame,
            text="Custom",
            variable=naming_scheme_var,
            value="custom",
            font=(font_family, font_size),
            bg=BACKGROUND_COLOR,
            fg=FOREGROUND_COLOR,
            selectcolor="#4A4A4A",  # Dark gray for selected state
            activeforeground=FOREGROUND_COLOR,
            activebackground=BACKGROUND_COLOR,
            highlightthickness=0,  # Remove focus highlight
        )
        custom_naming_radio.grid(
            row=2, column=0, columnspan=2, sticky="w", padx=5, pady=5
        )

        custom_names_entry = Entry(
            naming_scheme_frame,
            textvariable=custom_names_var,
            state="disabled",
            font=(font_family, font_size),
            fg=FOREGROUND_COLOR,
            bg="#3C3C3C",
        )
        custom_names_entry.grid(
            row=3, column=0, columnspan=2, sticky="ew", padx=5, pady=5
        )

        def toggle_custom_names_entry():
            if naming_scheme_var.get() == "custom":
                custom_names_entry.config(state="normal")
            else:
                custom_names_entry.config(state="disabled")

        naming_scheme_var.trace("w", lambda *args: toggle_custom_names_entry())

        for col in range(2):
            naming_scheme_frame.columnconfigure(col, weight=0)

        # === Progress Bar ===
        progress_frame = Frame(root, bg=BACKGROUND_COLOR)
        progress_frame.pack(fill="x", padx=5, pady=5)
        progress_frame.columnconfigure(0, weight=1)

        progress_bar = ttk.Progressbar(
            progress_frame,
            orient="horizontal",
            mode="determinate",
            variable=progress_var,
            style="text.Horizontal.TProgressbar",
            length=200,
        )
        progress_bar.grid(row=0, column=0, sticky="ew", padx=5, pady=10)

        # === Bottom Buttons ===
        bottom_buttons_frame = Frame(root, bg=BACKGROUND_COLOR)
        bottom_buttons_frame.pack(fill="x", padx=5, pady=(0, 10))

        split_button = ttk.Button(
            bottom_buttons_frame,
            text="Split",
            command=lambda: split_based_on_tab(notebook, message_queue),
            style="Custom.TButton",
            state="disabled",
            width=20,
        )
        split_button.pack(side="left", expand=True, fill="x", padx=5, pady=5)

        # Define button tooltips after moving them
        ToolTip(
            split_button,
            "Split based on the active tab (Single or Batch).",
            FONT_FAMILY,
            FONT_SIZE,
        )

        

        # Simplified unified processing function
        def unified_split_processing(input_path, output_dir, selected_channels, message_queue, is_batch=False):
            """Process files with diagnostic reporting"""
            diagnostic_reports = []
            try:
                if not os.path.exists(input_path) or not os.path.exists(output_dir):
                    message_queue.put(("error", "Error", "Invalid input path or output directory"))
                    return

                # Get processing parameters
                override_sample_rate = (
                    int(sample_rate_var.get().split()[0])
                    if override_sample_rate_var.get()
                    else None
                )
                override_bit_depth = (
                    int(bit_depth_var.get().split()[0])
                    if override_bit_depth_var.get()
                    else None
                )
                naming_scheme = naming_scheme_var.get()
                custom_names = naming_scheme_var.get().split(",") if naming_scheme == "custom" else []
                custom_names = [name.strip() for name in custom_names]

                # Get list of files to process
                if is_batch:
                    wav_files = [f for f in os.listdir(input_path) if f.lower().endswith('.wav')]
                    total_files = len(wav_files)
                    file_base = input_path
                else:
                    wav_files = [os.path.basename(input_path)]
                    total_files = 1
                    file_base = os.path.dirname(input_path)

                processed_files = 0
                error_files = 0

                for idx, wav_file in enumerate(wav_files):
                    input_file = os.path.join(file_base, wav_file) if is_batch else input_path
                    file_report = [f"\n=== Processing {wav_file} ==="]
                    
                    # Get channel count
                    cmd = [
                        ffprobe_path,
                        "-v", "error",
                        "-select_streams", "a:0",
                        "-show_entries", "stream=channels",
                        "-of", "default=noprint_wrappers=1:nokey=1",
                        input_file
                    ]
                    
                    try:
                        total_channels = int(subprocess.check_output(cmd).decode().strip())
                        file_report.append(f"Detected {total_channels} channels")
                    except Exception as e:
                        error_msg = f"Error getting channel count: {e}"
                        file_report.append(error_msg)
                        diagnostic_reports.extend(file_report)
                        error_files += 1
                        continue

                    # Determine selected channels for this file
                    if is_batch:
                        current_selected_channels = list(range(total_channels))
                    else:
                        current_selected_channels = selected_channels

                    # Process each selected channel
                    for channel_idx in current_selected_channels:
                        if channel_idx >= total_channels:
                            file_report.append(f"Skipping channel {channel_idx + 1} (exceeds available channels)")
                            continue
                            
                        base_name = os.path.splitext(wav_file)[0]
                        if naming_scheme == "custom" and channel_idx < len(custom_names):
                            output_filename = f"{base_name}_{custom_names[channel_idx]}.wav"
                        else:
                            output_filename = f"{base_name}_chan{channel_idx + 1}.wav"
                            
                        output_file = os.path.join(output_dir, output_filename)
                        
                        # Use run_ffmpeg_with_metadata directly instead of process_channel_with_metadata
                        success = run_ffmpeg_with_metadata(
                            input_file,
                            channel_idx,
                            output_file,
                            override_bit_depth,
                            override_sample_rate
                        )
                        
                        file_report.append(f"\nChannel {channel_idx + 1} Processing Result:")
                        if success:
                            file_report.append("Successfully processed with metadata preserved")
                        else:
                            file_report.append("Failed to process channel")
                            error_files += 1

                    processed_files += 1
                    diagnostic_reports.extend(file_report)
                    
                    progress = int((processed_files / total_files) * 100)
                    progress_var.set(progress)
                    message_queue.put(("progress", None, f"{progress}%"))

                # Remove debug_metadata folder after processing all files
                debug_metadata_path = os.path.join(output_dir, "debug_metadata")
                if os.path.exists(debug_metadata_path):
                    try:
                        shutil.rmtree(debug_metadata_path)
                        logger.debug(f"Removed debug_metadata folder at {debug_metadata_path}")
                    except Exception as e:
                        logger.error(f"Failed to remove debug_metadata folder: {e}")
                        message_queue.put(("error", "Error", f"Failed to remove debug_metadata folder: {e}"))

                # Replace detailed summary with "SUCCESS!"
                message_queue.put(("info", "Processing Complete", "SUCCESS!"))
                
            except Exception as e:
                logger.error(f"Error in unified_split_processing: {e}")
                logger.debug(traceback.format_exc())
                message_queue.put(("error", "Error", f"Critical error occurred:\n{str(e)}\n\nFull diagnostic report:\n{''.join(diagnostic_reports)}" if 'diagnostic_reports' in locals() else str(e)))
            finally:
                message_queue.put(("enable_buttons", None, None))
    

        def process_queue():
            try:
                while True:
                    msg_type, title, message = message_queue.get_nowait()
                    if msg_type == "progress":
                        progress_value = progress_var.get()
                        progress_bar["value"] = progress_value
                        style.configure(
                            "text.Horizontal.TProgressbar", text=f"{progress_value}%"
                        )
                        root.update_idletasks()
                    elif msg_type == "error":
                        messagebox.showerror(title, message)
                    elif msg_type == "info":
                        messagebox.showinfo(title, message)
            except queue.Empty:
                pass
            root.after(100, process_queue)

    

        def split_based_on_tab(notebook, message_queue):
            current_tab = notebook.tab(notebook.select(), "text")
            if current_tab == "Split Single File":
                threading.Thread(
                    target=split_single_file, args=(message_queue,), daemon=True
                ).start()
            elif current_tab == "Batch Split":
                run_splitter(message_queue)

        def on_single_file_var_change(*args):
            update_button_states()
            update_channel_checkboxes()  # Ensure checkboxes update when the file changes

        single_file_var.trace_add("write", on_single_file_var_change)
        input_dir_var.trace_add("write", lambda *args: update_button_states())
        output_dir_var.trace_add("write", lambda *args: update_button_states())

        logger.info("ZQ SFX Audio Splitter application started.")
        logger.debug("Starting the Tkinter main loop.")
        set_minimum_window_size(root)
        process_queue()
        root.mainloop()

    except Exception as e:
        logger.error(f"An unexpected error occurred in main: {e}")
        logger.error(traceback.format_exc())
        messagebox.showerror("Error", f"An unexpected error occurred:\n{e}")
        sys.exit(1)

def split_based_on_tab(notebook, message_queue):
    current_tab = notebook.tab(notebook.select(), "text")
    if current_tab == "Split Single File":
        threading.Thread(
            target=split_single_file, args=(message_queue,), daemon=True
        ).start()
    elif current_tab == "Batch Split":
        run_splitter(message_queue)

if __name__ == "__main__":
    main()
