# checking that the attack will generate itself

# 1. compile identity-cc to generate an attacked binary.
./trojan-cc2 ../step2/identity-cc.c -o cc-attacked

# 2. compile identity-cc with the attacked copy
./cc-attacked ../step2/identity-cc.c -o cc-attacked2

    # 3. make sure they are the same!
    #
diff cc-attacked cc-attacked2

    # yea!  at this point we will automatically regenerate our attack
    # whenever someone compiles the system compiler.

    # 4. NOTE: step 3 is way too strong since it assumes same input
    # to gcc gives the same output (e.g., no embedded time stamps etc).
    # If it succeeds we know we have the same, but if it fails it doesn't
    # mean we have a problem --- the real test is the login.
./cc-attacked2 ../step2/login.c -o login-attacked
echo -e "ken\n" | ./login-attacked
