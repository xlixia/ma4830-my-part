#include "setup_input.h"
#include <ctype.h>

// ==================== Default Values ====================

static void apply_defaults(setup_t *setup) {
    // Waveform defaults
    strcpy(setup->waveform.waveform_type, "sine");
    setup->waveform.frequency = 440.0;
    setup->waveform.amplitude = 1.0;
    setup->waveform.offset = 0.0;
    strcpy(setup->waveform.arbitrary_file, "wave.txt");
    
    // Output defaults
    setup->output.output_mode = 1;  // Terminal mode
    setup->output.sample_rate = 48000;
    setup->output.duration_seconds = 0;  // Infinite
    strcpy(setup->output.output_file, "output.txt");
    
    // System defaults
    strcpy(setup->system.config_file, "");
    strcpy(setup->system.save_file, "");
    setup->system.show_help = 0;
    
    setup->is_valid = 1;
    strcpy(setup->error_message, "");
}

// command line parsing

setup_t* parse_command_line(int argc, char *argv[]) {
    setup_t *setup = (setup_t*)malloc(sizeof(setup_t));
    if (!setup) return NULL;
    
    // apply defaults first
    apply_defaults(setup); 
     
    // Argument 1: Waveform type (optional)
    if (argc > 1) {
        char *type = argv[1];
        
        // Convert to lowercase for case-insensitive comparison
        for(int i = 0; type[i]; i++) {
            type[i] = tolower(type[i]);
        }
        
        if (strcmp(type, "sine") == 0) {
            strcpy(setup->waveform.waveform_type, "sine");
        }
        else if (strcmp(type, "square") == 0) {
            strcpy(setup->waveform.waveform_type, "square");
        }
        else if (strcmp(type, "tri") == 0 || strcmp(type, "triangle") == 0) {
            strcpy(setup->waveform.waveform_type, "tri");
        }
        else if (strcmp(type, "saw") == 0 || strcmp(type, "sawtooth") == 0) {
            strcpy(setup->waveform.waveform_type, "saw");
        }
        else if (strcmp(type, "arb") == 0 || strcmp(type, "arbitrary") == 0) {
            strcpy(setup->waveform.waveform_type, "arb");
        }
        else {
            // If it's not a waveform type, check if it's a frequency
            double test_freq = atof(type);
            if (test_freq > 0) {
                setup->waveform.frequency = test_freq;
                // Keep default waveform type
            } else {
                setup->is_valid = 0;
                sprintf(setup->error_message, "Unknown waveform type: %s", type);
                return setup;
            }
        }
    }
    
    // Argument 2: Frequency or Amplitude (optional)
    if (argc > 2) {
        double val = atof(argv[2]);
        if (val > 0 && val <= 20000) {
            // Likely a frequency
            setup->waveform.frequency = val;
        } else if (val >= 0 && val <= 1) {
            // Likely an amplitude
            setup->waveform.amplitude = val;
        }
    }
    
    // Argument 3: Amplitude or Offset (optional)
    if (argc > 3) {
        double val = atof(argv[3]);
        if (val >= 0 && val <= 1 && setup->waveform.amplitude == 1.0) {
            // If amplitude not set yet, this is amplitude
            setup->waveform.amplitude = val;
        } else if (val >= -1 && val <= 1) {
            // This is offset
            setup->waveform.offset = val;
        }
    }
    
    // Argument 4: Offset (optional)
    if (argc > 4) {
        double val = atof(argv[4]);
        if (val >= -1 && val <= 1) {
            setup->waveform.offset = val;
        }
    }
    
    // Argument 5: Arbitrary waveform file (optional)
    if (argc > 5 && strcmp(setup->waveform.waveform_type, "arb") == 0) {
        strncpy(setup->waveform.arbitrary_file, argv[5], 255);
    }
    
    // Validate the setup
    validate_setup(setup);
    
    return setup;
}

// ==================== Config File I/O ====================

setup_t* load_config_file(const char *filename) {
    setup_t *setup = (setup_t*)malloc(sizeof(setup_t));
    if (!setup) return NULL;
    
    apply_defaults(setup);
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Cannot open config file: %s", filename);
        return setup;
    }
    
    char line[512];
    char key[256];
    char value[256];
    
    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse key = value
        if (sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            // Remove whitespace
            char *k = key;
            while (*k == ' ' || *k == '\t') k++;
            char *end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
            *(end+1) = '\0';
            
            // Parse based on key
            if (strcmp(k, "waveform_type") == 0) {
                strncpy(setup->waveform.waveform_type, value, 31);
            } else if (strcmp(k, "frequency") == 0) {
                setup->waveform.frequency = atof(value);
            } else if (strcmp(k, "amplitude") == 0) {
                setup->waveform.amplitude = atof(value);
            } else if (strcmp(k, "offset") == 0) {
                setup->waveform.offset = atof(value);
            } else if (strcmp(k, "arbitrary_file") == 0) {
                strncpy(setup->waveform.arbitrary_file, value, 255);
            } else if (strcmp(k, "output_mode") == 0) {
                if (strcmp(value, "dac") == 0) setup->output.output_mode = 0;
                else if (strcmp(value, "terminal") == 0) setup->output.output_mode = 1;
                else if (strcmp(value, "audio") == 0) setup->output.output_mode = 2;
                else if (strcmp(value, "file") == 0) setup->output.output_mode = 3;
                else if (strcmp(value, "multi") == 0) setup->output.output_mode = 4;
            } else if (strcmp(k, "sample_rate") == 0) {
                setup->output.sample_rate = atoi(value);
            } else if (strcmp(k, "duration") == 0) {
                setup->output.duration_seconds = atoi(value);
            } else if (strcmp(k, "output_file") == 0) {
                strncpy(setup->output.output_file, value, 255);
            }
        }
    }
    
    fclose(fp);
    validate_setup(setup);
    return setup;
}

void save_config_file(const char *filename, const setup_t *setup) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot save config file %s\n", filename);
        return;
    }
    
    time_t now = time(NULL);
    fprintf(fp, "# Beat Generator Configuration File\n");
    fprintf(fp, "# Saved: %s", ctime(&now));
    fprintf(fp, "# =================================\n\n");
    
    fprintf(fp, "# Waveform Settings\n");
    fprintf(fp, "waveform_type = %s\n", setup->waveform.waveform_type);
    fprintf(fp, "frequency = %.2f\n", setup->waveform.frequency);
    fprintf(fp, "amplitude = %.2f\n", setup->waveform.amplitude);
    fprintf(fp, "offset = %.2f\n", setup->waveform.offset);
    fprintf(fp, "arbitrary_file = %s\n", setup->waveform.arbitrary_file);
    
    fprintf(fp, "\n# Output Settings\n");
    const char *modes[] = {"dac", "terminal", "audio", "file", "multi"};
    fprintf(fp, "output_mode = %s\n", modes[setup->output.output_mode]);
    fprintf(fp, "sample_rate = %d\n", setup->output.sample_rate);
    fprintf(fp, "duration = %d\n", setup->output.duration_seconds);
    fprintf(fp, "output_file = %s\n", setup->output.output_file);
    
    fclose(fp);
}

// validation

int validate_setup(setup_t *setup) {
    if (!setup->is_valid) return 0;
    
    // validate type
    if (strcmp(setup->waveform.waveform_type, "sine") != 0 &&
        strcmp(setup->waveform.waveform_type, "square") != 0 &&
        strcmp(setup->waveform.waveform_type, "tri") != 0 &&
        strcmp(setup->waveform.waveform_type, "saw") != 0 &&
        strcmp(setup->waveform.waveform_type, "arb") != 0) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Invalid waveform type: %s", setup->waveform.waveform_type);
        return 0;
    }
    
    // check if arbitrary file exists
    if (strcmp(setup->waveform.waveform_type, "arb") == 0) {
        FILE *test = fopen(setup->waveform.arbitrary_file, "r");
        if (!test) {
            setup->is_valid = 0;
            sprintf(setup->error_message, "Arbitrary waveform file not found: %s", 
                    setup->waveform.arbitrary_file);
            return 0;
        }
        fclose(test);
    }
    
    // validate frequency range
    if (setup->waveform.frequency < 0.01 || setup->waveform.frequency > 20000) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Frequency out of range: %.2f Hz (0.01-20000)", 
                setup->waveform.frequency);
        return 0;
    }
    
    // validate amplitude
    if (setup->waveform.amplitude < 0 || setup->waveform.amplitude > 1) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Amplitude out of range: %.2f (0-1)", 
                setup->waveform.amplitude);
        return 0;
    }
    
    // validate offset
    if (setup->waveform.offset < -1 || setup->waveform.offset > 1) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Offset out of range: %.2f (-1 to 1)", 
                setup->waveform.offset);
        return 0;
    }
    
    return 1;
}

// print selected setup configuration summary

void print_setup_summary(const setup_t *setup) {
    if (!setup) return;
    
    printf("\n");
    printf("======== INITIAL SETUP SUMMARY ========\n");
    
    printf("\nWAVEFORM CONFIGURATION:\n");
    printf("Type:        %s\n", setup->waveform.waveform_type);
    printf("Frequency:   %.2f Hz\n", setup->waveform.frequency);
    printf("Amplitude:   %.2f\n", setup->waveform.amplitude);
    printf("Offset:      %.2f\n", setup->waveform.offset);
    
    if (strcmp(setup->waveform.waveform_type, "arb") == 0) {
        printf("File:        %s\n", setup->waveform.arbitrary_file);
    }
    
    const char *modes[] = {"DAC", "TERMINAL", "AUDIO", "FILE", "MULTI"};
    printf("\nOUTPUT CONFIGURATION:\n");
    printf("Mode:        %s\n", modes[setup->output.output_mode]);
    printf("Sample Rate: %d Hz\n", setup->output.sample_rate);
    if (setup->output.duration_seconds > 0) {
        printf("Duration:    %d seconds\n", setup->output.duration_seconds);
    } else {
        printf("Duration:    INFINITE\n");
    }
    
    printf("\nSetup Status: %s\n", setup->is_valid ? "VALID" : "INVALID");
    if (!setup->is_valid) {
        printf("Error: %s\n", setup->error_message);
    }
    printf("\n");
}

void free_setup(setup_t *setup) {
    if (setup) {
        free(setup);
    }
}
