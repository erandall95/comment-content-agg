export default function validateURL(input: string): {isValid: boolean; id: string;} {
    let isValid = false
    let id = ""
    // get id from url, it's after "?v=" (https://www.youtube.com/watch?v=rkMJCVjNtoM)
    let index = input.indexOf("?v=")
    isValid = index !== -1
    if(isValid) {
        id = input.substring(index + 3)
    }
    return {isValid, id}
}