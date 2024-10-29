# Define the command completion function
_mycontext_complete() {
    local command="${COMP_WORDS[1]}"
    
    case "$command" in
        init)
            COMPREPLY=($(compgen -W "" -- "${COMP_WORDS[2]}"))
            ;;
        status)
            COMPREPLY=($(compgen -W "--recursive" -- "${COMP_WORDS[2]}"))
            ;;
        store) 
            # TODO
            COMPREPLY=($(compgen -W "$(ls)" -- "${COMP_WORDS[2]}"))
            ;;
        check)
            COMPREPLY=($(compgen -W "$(ls)" -- "${COMP_WORDS[2]}"))
            ;;
        check-tree)
            COMPREPLY=($(compgen -W "$(ls)" -- "${COMP_WORDS[2]}"))
            ;;
        wipe)
            COMPREPLY=($(compgen -W "$(ls)" -- "${COMP_WORDS[2]}"))
            ;;
        normalize-files)
            COMPREPLY=($(compgen -W "$(ls)" -- "${COMP_WORDS[2]}"))
            ;;
        hash)
            COMPREPLY=($(compgen -W "-f --force" -- "${COMP_WORDS[2]}"))
            ;;
        *)
            COMPREPLY=($(compgen -W "init status store check check-tree wipe normalize-files hash" -- "$command"))
            ;;
    esac
}

# Register the autocomplete function with the command
complete -F _mycontext_complete sht
