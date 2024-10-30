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
      store|check|check-tree|wipe|normalize-files)
         # Use globbing to list files instead of calling `ls`
         COMPREPLY=($(compgen -W "$(echo *)" -- "${COMP_WORDS[2]}"))
         ;;
      # TODO "sht tag<tab>" gives "sht foo-tag " instead of "sht tag "
      tag )
         COMPREPLY=($(compgen -W "$(ls .sht/tags 2>/dev/null)" -- "${COMP_WORDS[2]}"))
         ;;
      hash)
         COMPREPLY=($(compgen -W "-f --force" -- "${COMP_WORDS[2]}"))
         ;;
      *)
         COMPREPLY=($(compgen -W "init status store check check-tree wipe normalize-files tag hash" -- "$command"))
         ;;
   esac
}

# Register the autocomplete function with the command
complete -F _mycontext_complete sht
